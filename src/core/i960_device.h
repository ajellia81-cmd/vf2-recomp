// MAME's Intel 80960KB core (third_party/mame/i960; its members made protected, nothing else
// changed) on the shim's plain bus, with named access to the registers it publishes through
// state_add.
//
// The recompiler (src/rec) runs the game's code in the interpreter's place when one is attached
// (set_rec): recompiled functions for the code it knows, MAME's interpreter for the rest, the
// same state and the same cycle counts either way. set_rec(nullptr) puts the interpreter back.
#pragma once
#include "emu.h"
#include "../../third_party/mame/i960/i960.h"
#include <functional>
#include <vector>

struct i960_rec;
namespace i960rec { struct runtime; }

class i960_device : public i80960kb_device
{
	friend struct i960_rec;
public:
	explicit i960_device(mame_bus32 &bus) : i80960kb_device(s_config, "maincpu", nullptr, 25000000)
	{
		set_bus(&bus);
		start();
		for (int i = 0; i < 38; i++) m_reg[i] = state_find(i);
	}

	// I960_R0..I960_G15 (0-31), I960_SAT, I960_PRCB, I960_PC, I960_AC, I960_IP, I960_PIP
	u32 reg(int i) const { return u32(m_reg[i]->value()); }
	void set_reg(int i, u32 v) { m_reg[i]->set_value(v); }
	u32 ip() const { return reg(I960_IP); }       // the next instruction
	u32 pip() const { return reg(I960_PIP); }     // the instruction being executed
	u32 pc_reg() const { return reg(I960_PC); }   // process controls (priority in bits 16-20)

	// The recompiled code (src/rec/i960_rec.h), or null: the interpreter
	void set_rec(i960rec::runtime *r) { m_rec = r; }
	i960rec::runtime *rec() const { return m_rec; }

	// Everything the core keeps, for comparing two of them (the lockstep check): the registers,
	// the register cache, the floating-point registers, the interrupt and stall state, the
	// cycle count.
	void state_words(std::vector<u32> &out) const;
	// After each slice the scheduler gives it (testing: the lockstep check's finer comparison)
	std::function<void()> on_slice;
	// Time spent running it (measuring: counted while time_runs is set), in ns
	bool time_runs = false;
	uint64_t run_ns = 0, runs = 0;

protected:
	void execute_run() override;

private:
	static inline machine_config s_config;
	const device_state_entry *m_reg[38];
	i960rec::runtime *m_rec = nullptr;
};
