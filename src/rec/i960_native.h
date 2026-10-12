// The recompile done on the player's machine: VF2's i960 program, found with the shipped address
// map (assets/vf2_map.txt, src/rec/i960_analyse.h), made into native code (ARM64 or x86-64) with
// sljit when the game starts. Nothing made from the ROM ships; this is the job tools/recomp does
// into C++ for our own builds, and the lockstep check (vf2lock --native) covers it the same way.
#pragma once
#include "i960_rec.h"
#include "i960_analyse.h"

namespace i960rec {

struct native_stats { size_t functions = 0, instructions = 0, native = 0, code_bytes = 0; double ms = 0; };

// rd: the program as the CPU sees it (the program ROM, 2 MB). Makes every function of the map's
// program into native code and installs it in rt. False on failure (the interpreter runs it all).
bool build_native(runtime &rt, uint32_t (*rd)(void *, uint32_t), void *ctx, uint32_t limit, const hints &map, native_stats *stats = nullptr);

} // namespace i960rec
