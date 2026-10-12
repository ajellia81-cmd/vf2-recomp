// Stand-in for MAME's emu.h: just enough of the device framework for MAME's CPU core files
// (third_party/mame/...) to compile unchanged. A core's memory goes to a plain bus interface,
// and its registers (whatever it hands to state_add) can be read and written by index.
#pragma once
#include "../mame_shim.h"
#include "../dasm_shim.h"
#include <map>
#include <memory>
#include <vector>

#define ATTR_COLD
#define NAME(x) x, #x
#define DWORD_ALIGNED(a) (((a) & 3) == 0)
#define WORD_ALIGNED(a) (((a) & 1) == 0)

enum { AS_PROGRAM = 0, AS_DATA = 1, AS_IO = 2, AS_OPCODES = 3 };
enum endianness_t { ENDIANNESS_LITTLE, ENDIANNESS_BIG };
enum { STATE_GENPC = -1, STATE_GENPCBASE = -2, STATE_GENFLAGS = -3, STATE_GENSP = -4 };

struct machine_config {};
class device_t;
struct device_type_tag {};
using device_type = const device_type_tag *;
#define DECLARE_DEVICE_TYPE(Type, Class) extern device_type Type;
#define DEFINE_DEVICE_TYPE(Type, Class, ShortName, FullName) static const device_type_tag Type##_tag{}; device_type Type = &Type##_tag;

struct address_space_config
{
	address_space_config() = default;
	address_space_config(const char *, endianness_t, int, int, int) {}
};

struct device_memory_interface
{
	using space_config_vector = std::vector<std::pair<int, const address_space_config *>>;
};

// A 32-bit little-endian bus. The flags are MAME's per-region map flags (the i960 core's BURST).
struct mame_bus32
{
	virtual ~mame_bus32() = default;
	virtual u8 read_byte(offs_t a) = 0;
	virtual u16 read_word(offs_t a) = 0;
	virtual u32 read_dword(offs_t a) = 0;
	virtual void write_byte(offs_t a, u8 v) = 0;
	virtual void write_word(offs_t a, u16 v) = 0;
	virtual void write_dword(offs_t a, u32 v) = 0;
	virtual u16 flags(offs_t a) { return 0; }
};

template <int Width, int AddrShift, int Level, endianness_t Endian> struct memory_access
{
	struct specific
	{
		mame_bus32 *bus = nullptr;
		u8 read_byte(offs_t a) { return bus->read_byte(a); }
		u16 read_word(offs_t a) { return bus->read_word(a); }
		u32 read_dword(offs_t a) { return bus->read_dword(a); }
		void write_byte(offs_t a, u8 v) { bus->write_byte(a, v); }
		void write_word(offs_t a, u16 v) { bus->write_word(a, v); }
		void write_dword(offs_t a, u32 v) { bus->write_dword(a, v); }
		std::pair<u8, u16> read_byte_flags(offs_t a) { u16 f = bus->flags(a); return { bus->read_byte(a), f }; }
		std::pair<u32, u16> read_dword_flags(offs_t a) { u16 f = bus->flags(a); return { bus->read_dword(a), f }; }
		u16 write_byte_flags(offs_t a, u8 v) { u16 f = bus->flags(a); bus->write_byte(a, v); return f; }
		u16 write_dword_flags(offs_t a, u32 v) { u16 f = bus->flags(a); bus->write_dword(a, v); return f; }
	};
	struct cache : specific {};
};

struct address_space
{
	mame_bus32 *bus = nullptr;
	template <typename T> void cache(T &c) { c.bus = bus; }
	template <typename T> void specific(T &s) { s.bus = bus; }
};

// A register as the debugger sees it (state_add): where it lives and how wide it is.
class device_state_entry
{
public:
	device_state_entry(int index, const char *name, void *ptr, int size) : m_index(index), m_name(name), m_ptr(ptr), m_size(size) {}
	device_state_entry &formatstr(const char *) { return *this; }
	device_state_entry &noshow() { return *this; }
	device_state_entry &mask(u64) { return *this; }
	device_state_entry &callimport() { return *this; }
	device_state_entry &callexport() { return *this; }
	int index() const { return m_index; }
	const char *name() const { return m_name; }
	u64 value() const
	{
		switch (m_size) {
		case 1: return *static_cast<u8 *>(m_ptr);
		case 2: return *static_cast<u16 *>(m_ptr);
		case 4: return *static_cast<u32 *>(m_ptr);
		default: return *static_cast<u64 *>(m_ptr);
		}
	}
	void set_value(u64 v) const
	{
		switch (m_size) {
		case 1: *static_cast<u8 *>(m_ptr) = u8(v); break;
		case 2: *static_cast<u16 *>(m_ptr) = u16(v); break;
		case 4: *static_cast<u32 *>(m_ptr) = u32(v); break;
		default: *static_cast<u64 *>(m_ptr) = v; break;
		}
	}
private:
	int m_index;
	const char *m_name;
	void *m_ptr;
	int m_size;
};

// The CPU device base: what a MAME core derives from. The machine drives it through this base:
// set_bus(), start(), reset(), run(cycles), set_input_line(), and the state table.
class cpu_device
{
public:
	cpu_device(const machine_config &, device_type, const char *, device_t *, uint32_t clock) : m_clock(clock) {}
	virtual ~cpu_device() = default;

	using space_config_vector = device_memory_interface::space_config_vector;

	void set_bus(mame_bus32 *bus) { m_space.bus = bus; }
	void start() { device_start(); }
	void reset() { device_reset(); }
	// Runs until the cycle budget is spent (MAME's execute_run with icount = cycles); returns
	// the cycles actually used (an instruction may overrun the budget, as on MAME).
	int run(int cycles)
	{
		m_cycles_running = cycles;
		*m_icountptr = cycles;
		execute_run();
		return m_cycles_running - *m_icountptr;
	}
	// Cycles used so far in the current run().
	int cycles_used() const { return m_cycles_running - *m_icountptr; }
	// Ends the current run after this instruction (MAME's abort_timeslice: the rest of the
	// budget is given back, not spent).
	void abort_timeslice()
	{
		if (!m_icountptr) return;
		const int left = *m_icountptr;
		m_cycles_running -= left;
		*m_icountptr -= left;
	}
	int icount() const { return *m_icountptr; }
	void adjust_icount(int n) { *m_icountptr += n; }

	// The cycles since power-on (total_cycles): the machine sets the count at the start of each
	// run(); the cycles used so far in it are added.
	void set_total_base(u64 cycles) { m_total_base = cycles; }
	u64 total_cycles() const { return m_total_base + u64(cycles_used()); }

	// MAME's interruptible accesses (devcpu.cpp): a bus can delay an access, or have it redone
	// at the start of the next run when the delay would end this one.
	bool access_to_be_redone() noexcept { return std::exchange(m_access_to_be_redone, false); }
	bool access_to_be_redone_noclear() noexcept { return m_access_to_be_redone; }
	bool access_before_time(u64 access_time, u64 current_time) noexcept
	{
		s32 delta = s32(access_time - current_time);
		if (*m_icountptr <= delta) { defer_access(); return true; }
		*m_icountptr -= delta;
		return false;
	}
	bool access_before_delay(u32 cycles, const void *tag) noexcept
	{
		if (tag == m_access_before_delay_tag) { m_access_before_delay_tag = nullptr; return false; }
		*m_icountptr -= cycles;
		if (*m_icountptr <= 0) { m_access_before_delay_tag = tag; m_access_to_be_redone = true; return true; }
		m_access_before_delay_tag = nullptr;
		return false;
	}
	void access_after_delay(u32 cycles) noexcept { *m_icountptr -= cycles; }
	void defer_access() noexcept { if (*m_icountptr > 0) *m_icountptr = 0; m_access_to_be_redone = true; }
	void set_input_line(int line, int state) { execute_set_input(line, state); }

	const device_state_entry *state_find(int index) const
	{
		for (const auto &e : m_state) if (e.index() == index) return &e;
		return nullptr;
	}
	const device_state_entry *state_find(const char *name) const
	{
		for (const auto &e : m_state) if (!strcmp(e.name(), name)) return &e;
		return nullptr;
	}
	const std::vector<device_state_entry> &state_entries() const { return m_state; }
	std::unique_ptr<util::disasm_interface> disassembler() { return create_disassembler(); }

	// Called before each instruction (the debugger's hook), if set.
	std::function<void(offs_t)> on_instruction;
	// Called when the core takes an interrupt (standard_irq_callback).
	std::function<void(int, offs_t)> on_irq_ack;

protected:
	virtual void device_start() {}
	virtual void device_reset() {}
	virtual uint32_t execute_min_cycles() const noexcept { return 1; }
	virtual uint32_t execute_max_cycles() const noexcept { return 1; }
	virtual void execute_run() = 0;
	virtual void execute_set_input(int inputnum, int state) {}
	virtual space_config_vector memory_space_config() const { return {}; }
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const {}
	virtual std::unique_ptr<util::disasm_interface> create_disassembler() { return nullptr; }

	address_space &space(int) { return m_space; }
	template <typename T> device_state_entry &state_add(int index, const char *name, T &var)
	{
		m_state.emplace_back(index, name, static_cast<void *>(&var), int(sizeof(T)));
		return m_state.back();
	}
	template <typename T> void save_item(T &, const char *) {}
	template <typename T> void save_pointer(T *, const char *, size_t) {}
	void set_icountptr(int &icount) { m_icountptr = &icount; }
	void debugger_instruction_hook(offs_t pc) { if (on_instruction) on_instruction(pc); }
	void standard_irq_callback(int line, offs_t pc) { if (on_irq_ack) on_irq_ack(line, pc); }
	uint32_t clock() const { return m_clock; }

private:
	address_space m_space;
	std::vector<device_state_entry> m_state;
	int *m_icountptr = nullptr;
	int m_cycles_running = 0;
	u64 m_total_base = 0;
	bool m_access_to_be_redone = false;
	const void *m_access_before_delay_tag = nullptr;
	uint32_t m_clock;
};
