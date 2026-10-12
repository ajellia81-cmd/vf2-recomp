// Finds VF2's i960 code and splits it into functions: the first step of the recompile, done by
// tools/recomp/vf2recomp (into C++, for our own builds) and by the game itself at start-up (into
// native code, on the player's machine).
//
// From the ROM alone only the code reached by direct calls and branches is found; VF2 reaches
// much of its code through pointers (calls through registers, the scheduler's task table, jump
// tables, the interrupt table in RAM). The address map fills that in: where those indirect jumps
// and calls lead (recorded from test runs, vf2run --cov), and the "thunks" that return past data
// placed after the call (so the bytes after their call sites aren't code). Addresses only.
//
// Every instruction found belongs to exactly one function (the first whose body reaches it);
// a branch to another function's instruction leaves for that function.
#pragma once
#include "i960_decode.h"
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace i960rec {

struct hints
{
	std::set<uint32_t> entries;                     // functions: run-time call targets, interrupt handlers
	std::map<uint32_t, std::set<uint32_t>> edges;   // a -> where the change of flow at a went
	std::set<uint32_t> thunks;                      // call targets that don't return to the call's next instruction
	std::map<uint32_t, uint64_t> covered;           // instructions seen running (coverage files only), and how often

	// "entry A", "edge A B", "thunk A", "pc A N" lines (hex, N decimal); others ignored. Adds to
	// what's there.
	bool read(const std::string &path);
	void parse(const char *text);
};

// The address map this build was made with (src/rec/vf2_map.cpp, made by tools/recomp/vf2recomp)
extern const char vf2_map_text[];

struct function
{
	uint32_t entry = 0;
	std::vector<uint32_t> body;      // its instructions, in address order (the entry among them)
};

struct program
{
	std::map<uint32_t, decoded> insns;
	std::vector<function> functions;
	std::map<uint32_t, int> owner;                    // instruction -> function index
	std::map<uint32_t, std::vector<uint32_t>> hinted; // indirect jump -> its targets (the map's)
	std::set<uint32_t> thunks;
	// Spin loops: a run of loads from fixed addresses and compares, closed by a branch back to its
	// start (the idle loop waiting for the frame's interrupt). Once round, the loop does the same
	// thing every time while the memory it reads holds still, which it does for the whole of a
	// slice when it's plain memory (nothing else runs then). branch -> (head, cycles a time round)
	struct spin { uint32_t head, cost; std::vector<uint32_t> loads; };
	std::map<uint32_t, spin> spins;
	int owner_of(uint32_t a) const { auto it = owner.find(a); return it == owner.end() ? -1 : it->second; }
};

// rd(a): the program's 32-bit word at a (a < limit: the program ROM).
program analyse(const std::function<uint32_t(uint32_t)> &rd, uint32_t limit, const hints &h);

// The address map: every function's entry, the indirect jumps' and calls' targets, the thunks.
// The smallest hints that find the same program; no bytes from the ROM.
void write_map(const program &p, const hints &h, const char *path);

} // namespace i960rec
