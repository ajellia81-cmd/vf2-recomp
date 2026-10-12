// Included by the generated C++ (build/gen/vf2_rec.cpp, made by tools/recomp/vf2recomp): the
// statements each recompiled instruction is made of.
//   I(A, OP, DISP, LEN)  the instruction at A: its label, PIP and IP as the interpreter has them,
//                        then execute_op's case for it (i960_ops.h)
//   IH(...)              the same, at a spin loop's head (spun_A: it has run in this call)
//   STL                  after an access that may stall (the FIFO's read): the slice's loop
//                        retries it, as the interpreter's does
//   CHK                  the slice's cycles spent: back to the loop (IP is the next instruction)
//   TAIL(k)              on to function k (IP is where it goes)
//   DISP                 on to whatever function holds IP, or back to the loop
//   SPIN_PLAIN(a)        a spin loop's load reads plain memory (it holds still through the slice)
//   BAD(A)               an instruction the interpreter is to run (back to the loop with IP = A)
#pragma once
#include "i960_ops.h"
#include "i960_rec.h"

using C = i960_device;
using R = i960_rec;
#define IPV R::ip(c)
#define I(A, OP, DISP, LEN) L_##A: R::pip(c) = 0x##A; R::ip(c) = 0x##A + LEN; R::exec(c, OP, DISP);
#define IH(A, OP, DISP, LEN) L_##A: spun_##A = true; R::pip(c) = 0x##A; R::ip(c) = 0x##A + LEN; R::exec(c, OP, DISP);
#define STL if (R::stalled(c)) return;
#define CHK if (R::icount(c) <= 0) return;
#define TAIL(k) do { [[clang::musttail]] return c.rec()->fns[k](c); } while (0)
#define DISP do { [[clang::musttail]] return i960rec::dispatch(c); } while (0)
#define SPIN_PLAIN(a) (R::rp(c, a) && R::rp(c, (a) + 3))
#define BAD(A) L_##A: R::ip(c) = 0x##A; return;
