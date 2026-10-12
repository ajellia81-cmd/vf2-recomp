// The runtime for the recompiled i960 code: which function runs each address, the loop that
// stands in for the interpreter's execute_run, and the fall-back to MAME's interpreter for
// anything not recompiled.
//
// A recompiled function runs from wherever it's entered (a switch on IP) until the slice's
// cycles run out, an access stalls, or the flow leaves it (a call, a return, an indirect jump,
// a branch to another function's code); it then returns, or goes straight on to the next
// function (a tail call). The i960's own state is all in the core (registers, the register
// cache, IP), so nothing is kept on the host's stack between slices: a slice can end at any
// instruction, as the interpreter's do, and the next one carries on from IP.
//
// The interpreter's order is kept exactly: an instruction is run only while the cycle count is
// above 0, interrupts are taken where the interpreter takes them (at the start of a slice, and
// by the instructions that look for them: ret from an interrupt, modpc, the IAC), and every
// instruction counts its cycles and makes its memory accesses as execute_op does.
#pragma once
#include "../core/i960_device.h"
#include "i960_analyse.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace i960rec {

using fn_t = void (*)(i960_device &);

// What a code generator hands the runtime: its functions, and each instruction it compiled
// with the word it was made from (checked against the program before use).
struct gen_insn { uint32_t addr, word, disp, fn; };   // disp: the displacement word (8-byte forms), else 0
struct gen_fn { uint32_t entry; fn_t code; };

struct stats
{
	size_t functions = 0, instructions = 0, dropped = 0;   // dropped: the program differs there
	uint64_t interpreted = 0;                              // instructions run by the interpreter (counted when asked)
};

struct runtime
{
	static constexpr uint32_t LIMIT = 0x200000;   // the program ROM
	std::vector<fn_t> table;     // per instruction address / 4: the function to enter there, or null
	// plain memory (no device behind it, no side effects, MAME's BURST flag), per 4 KB page below
	// PAGES << 12: where it is, for reads and for writes (null: through the bus)
	static constexpr uint32_t PAGES = 0x08000000 >> 12;
	std::vector<uint8_t *> rpage, wpage;
	std::vector<fn_t> fns;       // per function: its code, or stop() when dropped
	std::vector<gen_insn> insns; // what was compiled, by function (for code_changed)
	std::vector<fn_t> code;      // per function: its code, dropped or not
	std::vector<std::pair<uint32_t, uint32_t>> span;   // per function: its instructions in insns (first, count)
	uint32_t (*rd)(void *, uint32_t) = nullptr;   // the program as the CPU sees it now
	void *rd_ctx = nullptr;
	stats st;
	bool count_interp = false;   // count the instructions the interpreter runs (and where: interp_pc)
	std::vector<uint32_t> interp_pc;   // per address / 4 (when counting)

	// The native code (i960_native.h): one piece, entered at an instruction's label
	using nfn_t = void (*)(i960_device *, uintptr_t);
	i960_device *cpu = nullptr;           // the CPU it's made for (the native code has its fields' places)
	nfn_t nentry = nullptr;
	void *native_code = nullptr;
	std::vector<uintptr_t> nlabel;        // per instruction address / 4: its code, or 0
	std::shared_ptr<program> native_program;
	hints map;                            // (to make it again when the program changes)
	uint32_t limit = 0;
	~runtime();

	fn_t lookup(uint32_t ip) const { return ip < LIMIT && !(ip & 3) && !table.empty() ? table[ip >> 2] : nullptr; }

	// Installs a generator's functions. rd: the program as the CPU sees it now. Functions with an
	// instruction that isn't the one they were made from are left out (the interpreter runs them).
	void install(const gen_fn *f, size_t nf, const gen_insn *in, size_t ni, uint32_t (*rd)(void *, uint32_t), void *ctx);
	// The program changed at [addr, addr + n): the functions made from those bytes are dropped
	// (or taken back, when the bytes are the ones they were made from again).
	void code_changed(uint32_t addr, uint32_t n);
	bool matches(size_t k) const;     // function k's instructions are the program's now
	void enable(size_t k, bool on);
	void recount();

	// The interpreter's execute_run, with the recompiled code
	void execute(i960_device &c);
	static void stop(i960_device &);   // a dropped function: back to the loop (the interpreter)
};

// Goes on at IP: the function there, or back to the loop
void dispatch(i960_device &c);

// The C++ build's generated code (build/gen/vf2_rec.cpp), or none (src/rec/gen_none.cpp)
extern const gen_fn gen_fns[];
extern const size_t gen_nfns;
extern const gen_insn gen_insns[];
extern const size_t gen_ninsns;

} // namespace i960rec
