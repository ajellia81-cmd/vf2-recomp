// The native recompile (see i960_native.h). The whole program is one piece of code: a label at
// every instruction, entered at the one for IP, the flow between instructions as native jumps
// (into another function's code too: they share one frame), and back to the runtime's loop when
// the slice's cycles run out, an access stalls, or IP leads where no code was made.
//
// Each instruction stores PIP and IP as the interpreter has them, then does its work: through a
// call to execute_op's case for its opcode (i960_ops.h, made into one function per opcode and
// sub-opcode, the operands decoded from the word), or as native code for the common ones, with
// the same cycle counts and memory accesses as the interpreter's. After it: the stall check (a
// FIFO read retried by the loop, as the interpreter retries it), the cycle check, the flow.
#include "i960_native.h"
#include "i960_ops.h"
#include "../../third_party/sljit/sljitLir.h"
#include <chrono>
#include <map>
#include <unordered_map>
#include <utility>

namespace i960rec {

namespace {

using R = i960_rec;
using C = i960_device;

// ---- what the native code calls ----

// execute_op's case for one opcode (and, for the REG format, sub-opcode): the rest of the
// instruction word comes as an argument, the opcode is folded in
template <uint32_t MAJ, uint32_t SUB>
void SLJIT_FUNC h_op(C *c, sljit_u32 op, sljit_u32 disp)
{
	const uint32_t w = (MAJ >= 0x58 && MAJ < 0x80) ? (op & ~0xff000780u) | (MAJ << 24) | (SUB << 7) : (op & 0x00ffffffu) | (MAJ << 24);
	R::exec(*c, w, disp);
}

using op_fn = void(SLJIT_FUNC *)(C *, sljit_u32, sljit_u32);
op_fn g_ops[256][16];

template <uint32_t MAJ, uint32_t... SUB> void fill_subs(std::integer_sequence<uint32_t, SUB...>)
{
	((g_ops[MAJ][SUB] = &h_op<MAJ, SUB>), ...);
}
template <uint32_t MAJ> void fill_one()
{
	if constexpr (MAJ >= 0x58 && MAJ < 0x80) fill_subs<MAJ>(std::make_integer_sequence<uint32_t, 16>());
	else g_ops[MAJ][0] = &h_op<MAJ, 0>;
}
template <uint32_t... MAJ> void fill_ops(std::integer_sequence<uint32_t, MAJ...>) { (fill_one<MAJ>(), ...); }
// (the opcodes execute_op has: no others are compiled)
using all_majors = std::integer_sequence<uint32_t,
	0x08, 0x09, 0x0a, 0x0b, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x39, 0x3a, 0x3b, 0x3c,
	0x3d, 0x3e, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6c, 0x6d, 0x6e,
	0x70, 0x74, 0x78, 0x79, 0x80, 0x82, 0x84, 0x85, 0x86, 0x88, 0x8a, 0x8c, 0x90, 0x92, 0x98, 0x9a, 0xa0, 0xa2, 0xb0, 0xb2,
	0xc0, 0xc2, 0xc8, 0xca>;

op_fn op_for(uint32_t w)
{
	static const bool filled = (fill_ops(all_majors()), true);
	(void)filled;
	const uint32_t maj = w >> 24;
	return g_ops[maj][(maj >= 0x58 && maj < 0x80) ? (w >> 7) & 0xf : 0];
}

// Where the code for IP is (0: none: back to the loop)
sljit_uw SLJIT_FUNC h_label(C *c)
{
	const runtime &rt = *c->rec();
	const uint32_t ip = R::ip(*c);
	return ip < runtime::LIMIT && !(ip & 3) ? rt.nlabel[ip >> 2] : 0;
}

// A spin loop's skip (see program::spins): round again while the cycles last, all but the last
// time skipped, when the memory it reads is plain
void SLJIT_FUNC h_spin(C *c, sljit_u32 cost, sljit_uw loads)
{
	const auto &ls = *reinterpret_cast<const std::vector<uint32_t> *>(loads);
	int &ic = R::icount(*c);
	if (ic <= int(cost)) return;
	for (uint32_t a : ls) if (!R::rp(*c, a) || !R::rp(*c, a + 3)) return;
	ic -= (ic - 1) / int(cost) * int(cost);
}

struct gen
{
	sljit_compiler *C_ = nullptr;
	const program &p;
	runtime &rt;
	C &cpu;
	native_stats &st;
	std::unordered_map<uint32_t, sljit_label *> labels;
	std::vector<std::pair<sljit_jump *, uint32_t>> to_label;
	std::vector<sljit_jump *> to_exit, to_dispatch;
	sljit_sw o_ip = 0, o_pip = 0, o_icount = 0, o_stalled = 0;
	std::map<uint32_t, sljit_sw> spun;   // a spin loop's head -> its flag on the stack: it has run since the code was entered

	sljit_sw off(const void *field) const { return reinterpret_cast<const char *>(field) - reinterpret_cast<const char *>(&cpu); }

	bool compiled(uint32_t a) const
	{
		auto it = p.insns.find(a);
		return it != p.insns.end() && it->second.k != kind::bad && p.owner.count(a);
	}
	void go(uint32_t t)   // IP is t: carry on there
	{
		if (compiled(t)) to_label.push_back({ sljit_emit_jump(C_, SLJIT_JUMP), t });
		else to_dispatch.push_back(sljit_emit_jump(C_, SLJIT_JUMP));
	}
	void load_ip() { sljit_emit_op1(C_, SLJIT_MOV_U32, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S0), o_ip); }
	void if_ip(uint32_t t)   // IP (in R0) == t: carry on there
	{
		if (compiled(t)) to_label.push_back({ sljit_emit_cmp(C_, SLJIT_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, t), t });
		else to_dispatch.push_back(sljit_emit_cmp(C_, SLJIT_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, t));
	}

	// ---- the common instructions as native code (execute_op's cases, the same cycles) ----
	const bool native_ok = getenv("VF2_REC_CALLS") == nullptr;   // testing: every instruction through execute_op's case
	sljit_sw o_r = 0, o_ac = 0;
	uint8_t *const *rpage = nullptr, *const *wpage = nullptr;

	void call_op(const decoded &d)
	{
		sljit_emit_op1(C_, SLJIT_MOV, SLJIT_R0, 0, SLJIT_S0, 0);
		sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(d.op));
		sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, sljit_sw(d.len == 8 ? d.disp : 0));
		sljit_emit_icall(C_, SLJIT_CALL, SLJIT_ARGS3V(P, 32, 32), SLJIT_IMM, SLJIT_FUNC_ADDR(op_for(d.op)));
	}
	void stall_check()
	{
		sljit_emit_op1(C_, SLJIT_MOV_U8, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S0), o_stalled);
		to_exit.push_back(sljit_emit_cmp(C_, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0));
	}
	sljit_sw reg(int i) const { return o_r + 4 * i; }
	void ld_r(sljit_s32 R, int i) { sljit_emit_op1(C_, SLJIT_MOV_U32, R, 0, SLJIT_MEM1(SLJIT_S0), reg(i)); }
	void st_r(int i, sljit_s32 R) { sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), reg(i), R, 0); }
	void ic_sub(int k) { sljit_emit_op2(C_, SLJIT_SUB32, SLJIT_MEM1(SLJIT_S0), o_icount, SLJIT_MEM1(SLJIT_S0), o_icount, SLJIT_IMM, k); }
	void src1(sljit_s32 R, uint32_t op) { if (op & 0x800) sljit_emit_op1(C_, SLJIT_MOV32, R, 0, SLJIT_IMM, op & 0x1f); else ld_r(R, op & 0x1f); }
	void src2(sljit_s32 R, uint32_t op) { if (op & 0x1000) sljit_emit_op1(C_, SLJIT_MOV32, R, 0, SLJIT_IMM, (op >> 14) & 0x1f); else ld_r(R, (op >> 14) & 0x1f); }
	// get_ea into R1 (uses R0 too)
	bool ea(const decoded &d)
	{
		const uint32_t op = d.op;
		const int abase = (op >> 14) & 0x1f, index = op & 0x1f, scale = (op >> 7) & 7;
		if (!(op & 0x1000)) {
			const uint32_t offset = op & 0xfff;
			if (op & 0x2000) {
				ld_r(SLJIT_R1, abase);
				if (offset) sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, offset);
			} else
				sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, offset);
			return true;
		}
		auto indexed = [&](sljit_s32 R) {   // R = r[index] << scale
			ld_r(R, index);
			if (scale) sljit_emit_op2(C_, SLJIT_SHL32, R, 0, R, 0, SLJIT_IMM, scale);
		};
		switch ((op >> 10) & 0xf) {
		case 0x4: ld_r(SLJIT_R1, abase); return true;
		case 0x5: sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(uint32_t(d.disp + d.addr + 8))); return true;
		case 0x7: indexed(SLJIT_R0); ld_r(SLJIT_R1, abase); sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0); return true;
		case 0xc: sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(d.disp)); return true;
		case 0xd: ld_r(SLJIT_R1, abase); sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(d.disp)); return true;
		case 0xe: indexed(SLJIT_R1); sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(d.disp)); return true;
		case 0xf:
			indexed(SLJIT_R0); ld_r(SLJIT_R1, abase);
			sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
			sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(d.disp));
			return true;
		default: return false;
		}
	}
	// R2 = the page's memory for the address in R1 (u32), else a jump to `slow` (pushed): plain,
	// below the pages' limit, aligned to `align`
	void page(uint8_t *const *table, int align, std::vector<sljit_jump *> &slow)
	{
		if (align > 1) {
			sljit_emit_op2u(C_, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_IMM, align - 1);
			slow.push_back(sljit_emit_jump(C_, SLJIT_NOT_ZERO));
		}
		slow.push_back(sljit_emit_cmp(C_, SLJIT_GREATER_EQUAL | SLJIT_32, SLJIT_R1, 0, SLJIT_IMM, sljit_sw(runtime::PAGES << 12)));
		sljit_emit_op2(C_, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R1, 0, SLJIT_IMM, 12);
		sljit_emit_op1(C_, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, sljit_sw(table));
		sljit_emit_op1(C_, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM2(SLJIT_R2, SLJIT_R3), SLJIT_WORD_SHIFT);
		slow.push_back(sljit_emit_cmp(C_, SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0));
		sljit_emit_op2(C_, SLJIT_AND32, SLJIT_R3, 0, SLJIT_R1, 0, SLJIT_IMM, 0xfff);
	}
	// AC's condition code from comparing R0 (src1) with R1 (src2): 4 less, 2 equal, 1 greater
	void compare(bool sig)
	{
		sljit_emit_op2u(C_, SLJIT_SUB32 | SLJIT_SET_Z | (sig ? SLJIT_SET_SIG_LESS : SLJIT_SET_LESS), SLJIT_R0, 0, SLJIT_R1, 0);
		sljit_emit_op_flags(C_, SLJIT_MOV32, SLJIT_R2, 0, sig ? SLJIT_SIG_LESS : SLJIT_LESS);
		sljit_emit_op_flags(C_, SLJIT_MOV32, SLJIT_R3, 0, SLJIT_ZERO);
		sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
		sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R2, 0);
		sljit_emit_op2(C_, SLJIT_SHL32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 1);
		sljit_emit_op2(C_, SLJIT_ADD32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R2, 0);   // 1 + Z + 3 * less
		set_cc(SLJIT_R3);
	}
	void set_cc(sljit_s32 R)   // AC = (AC & ~7) | R
	{
		sljit_emit_op1(C_, SLJIT_MOV_U32, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_S0), o_ac);
		sljit_emit_op2(C_, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, ~7);
		sljit_emit_op2(C_, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, R, 0);
		sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_ac, SLJIT_R2, 0);
	}
	// if AC & mask (or, mask 0: AC & 7 is 0), IP = target
	void branch_if(uint32_t mask, uint32_t target)
	{
		sljit_emit_op1(C_, SLJIT_MOV_U32, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_S0), o_ac);
		sljit_emit_op2u(C_, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R2, 0, SLJIT_IMM, mask ? mask : 7);
		sljit_jump *no = sljit_emit_jump(C_, mask ? SLJIT_ZERO : SLJIT_NOT_ZERO);
		sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_ip, SLJIT_IMM, sljit_sw(target));
		sljit_set_label(no, sljit_emit_label(C_));
	}

	bool emit_inline(const decoded &d)
	{
		const uint32_t op = d.op, o = op >> 24, sub = (op >> 7) & 0xf, dst = (op >> 19) & 0x1f;
		const bool lit_dst = op & 0x2000;
		switch (o) {
		case 0x08:   // b
			ic_sub(1);
			sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_ip, SLJIT_IMM, sljit_sw(d.target));
			return true;
		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			ic_sub(1);
			branch_if(o & 7, d.target);
			return true;
		case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: {   // testxx
			ic_sub(1);
			sljit_emit_op1(C_, SLJIT_MOV_U32, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_S0), o_ac);
			sljit_emit_op2u(C_, SLJIT_AND32 | SLJIT_SET_Z, SLJIT_R2, 0, SLJIT_IMM, (o & 7) ? (o & 7) : 7);
			sljit_emit_op_flags(C_, SLJIT_MOV32, SLJIT_R0, 0, (o & 7) ? SLJIT_NOT_ZERO : SLJIT_ZERO);
			st_r(dst, SLJIT_R0);
			return true;
		}
		case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36:
		case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: {   // cmpob / cmpib and branch
			ic_sub(4);
			if (op & 0x2000) sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, (op >> 19) & 0x1f); else ld_r(SLJIT_R0, (op >> 19) & 0x1f);
			ld_r(SLJIT_R1, (op >> 14) & 0x1f);
			compare(o >= 0x39);
			branch_if(o & 7, d.target);
			return true;
		}
		case 0x30: case 0x37: {   // bbc, bbs
			ic_sub(4);
			if (op & 0x2000) sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, (op >> 19) & 0x1f); else ld_r(SLJIT_R0, (op >> 19) & 0x1f);
			ld_r(SLJIT_R1, (op >> 14) & 0x1f);
			sljit_emit_op2(C_, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0x1f);
			sljit_emit_op2(C_, SLJIT_LSHR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
			sljit_emit_op2(C_, SLJIT_AND32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);   // the bit
			// taken (bbc: clear, bbs: set): cc 2 and IP = target; else cc 0
			sljit_jump *notaken = sljit_emit_cmp(C_, SLJIT_EQUAL | SLJIT_32, SLJIT_R1, 0, SLJIT_IMM, o == 0x30 ? 1 : 0);
			sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R3, 0, SLJIT_IMM, 2);
			set_cc(SLJIT_R3);
			sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_ip, SLJIT_IMM, sljit_sw(d.target));
			sljit_jump *done = sljit_emit_jump(C_, SLJIT_JUMP);
			sljit_set_label(notaken, sljit_emit_label(C_));
			sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R3, 0, SLJIT_IMM, 0);
			set_cc(SLJIT_R3);
			sljit_set_label(done, sljit_emit_label(C_));
			return true;
		}
		case 0x58: {
			if (lit_dst) return false;
			int cost = 1;
			sljit_s32 alu = 0;
			bool inv1 = false, inv2 = false;
			switch (sub) {
			case 0x1: alu = SLJIT_AND32; break;                      // and
			case 0x2: alu = SLJIT_AND32; inv1 = true; break;         // andnot: t2 & ~t1
			case 0x4: alu = SLJIT_AND32; inv2 = true; break;         // notand: ~t2 & t1
			case 0x6: alu = SLJIT_XOR32; break;                      // xor
			case 0x7: alu = SLJIT_OR32; break;                       // or
			case 0xb: alu = SLJIT_OR32; inv1 = true; break;          // ornot: t2 | ~t1
			case 0xd: alu = SLJIT_OR32; inv2 = true; break;          // notor: ~t2 | t1
			case 0x3: case 0xc: case 0x0: {                          // setbit, clrbit, notbit
				ic_sub(2);
				src1(SLJIT_R0, op); src2(SLJIT_R1, op);
				sljit_emit_op2(C_, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 31);
				sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, 1);
				sljit_emit_op2(C_, SLJIT_SHL32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R0, 0);
				if (sub == 0x3) sljit_emit_op2(C_, SLJIT_OR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
				else if (sub == 0x0) sljit_emit_op2(C_, SLJIT_XOR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
				else { sljit_emit_op2(C_, SLJIT_XOR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, -1); sljit_emit_op2(C_, SLJIT_AND32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0); }
				st_r(dst, SLJIT_R1);
				return true;
			}
			default: return false;
			}
			ic_sub(cost);
			src1(SLJIT_R0, op); src2(SLJIT_R1, op);
			if (inv1) sljit_emit_op2(C_, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, -1);
			if (inv2) sljit_emit_op2(C_, SLJIT_XOR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, -1);
			sljit_emit_op2(C_, alu, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
			st_r(dst, SLJIT_R1);
			return true;
		}
		case 0x59: {
			if (lit_dst) return false;
			switch (sub) {
			case 0x0: case 0x1: case 0x2: case 0x3:   // addo addi subo subi
				ic_sub(1);
				src1(SLJIT_R0, op); src2(SLJIT_R1, op);
				sljit_emit_op2(C_, sub < 2 ? SLJIT_ADD32 : SLJIT_SUB32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
				st_r(dst, SLJIT_R1);
				return true;
			case 0x8: case 0xc: case 0xe: case 0xb: {   // shro, shlo, shli, shri: t1 >= 32 as MAME has it
				ic_sub(1);
				src1(SLJIT_R0, op); src2(SLJIT_R1, op);
				sljit_jump *big = sljit_emit_cmp(C_, SLJIT_GREATER_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, 32);
				sljit_emit_op2(C_, sub == 0x8 ? SLJIT_LSHR32 : sub == 0xb ? SLJIT_ASHR32 : SLJIT_SHL32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);
				sljit_jump *done = sljit_emit_jump(C_, SLJIT_JUMP);
				sljit_set_label(big, sljit_emit_label(C_));
				if (sub == 0xb) sljit_emit_op2(C_, SLJIT_ASHR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 31);
				else sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, 0);
				sljit_set_label(done, sljit_emit_label(C_));
				st_r(dst, SLJIT_R1);
				return true;
			}
			default: return false;
			}
		}
		case 0x5a:
			switch (sub) {
			case 0x0: case 0x1:   // cmpo, cmpi
				ic_sub(1);
				src1(SLJIT_R0, op); src2(SLJIT_R1, op);
				compare(sub == 1);
				return true;
			case 0x4: case 0x5: case 0x6: case 0x7:   // cmpinco, cmpinci, cmpdeco, cmpdeci
				if (lit_dst) return false;
				ic_sub(2);
				src1(SLJIT_R0, op); src2(SLJIT_R1, op);
				compare(sub & 1);
				src2(SLJIT_R1, op);
				sljit_emit_op2(C_, sub < 6 ? SLJIT_ADD32 : SLJIT_SUB32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
				st_r(dst, SLJIT_R1);
				return true;
			default: return false;
			}
		case 0x5c: case 0x5d: case 0x5e: case 0x5f: {   // mov, movl, movt, movq
			if (sub != 0xc) return false;
			const int n = int(o - 0x5c) + 1;
			const uint32_t t2 = o == 0x5c ? dst : o == 0x5d ? (op >> 19) & 0x1e : (op >> 19) & 0x1c;
			if (o == 0x5c && lit_dst) return false;
			const uint32_t s = op & 0x1f;
			if (!(op & 0x800) && n > 1 && ((s != t2 && s < t2 + n && t2 < s + n) || s + n > 32)) return false;   // (overlapping, or past g15: memcpy's)
			ic_sub(n == 1 ? 2 : n == 2 ? 2 : n);
			for (int i = 0; i < n; i++) {
				if (op & 0x800) sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, s);
				else ld_r(SLJIT_R0, int(s) + i);
				st_r(int(t2) + i, SLJIT_R0);
			}
			return true;
		}
		case 0x8c:   // lda
			if (!ea(d)) return false;
			ic_sub(1);
			st_r(dst, SLJIT_R1);
			return true;
		case 0x80: case 0xc0: case 0x88: case 0xc8: case 0x90: {   // ldob ldib ldos ldis ld
			if (!memb_mode_ok(op)) return false;
			const int size = o == 0x90 ? 4 : (o == 0x88 || o == 0xc8) ? 2 : 1;
			std::vector<sljit_jump *> slow;
			if (!ea(d)) return false;
			page(rpage, size, slow);
			ic_sub(4);
			const sljit_s32 mv = size == 4 ? SLJIT_MOV_U32 : o == 0x80 ? SLJIT_MOV_U8 : o == 0xc0 ? SLJIT_MOV_S8 : o == 0x88 ? SLJIT_MOV_U16 : SLJIT_MOV_S16;
			sljit_emit_op1(C_, mv, SLJIT_R0, 0, SLJIT_MEM2(SLJIT_R2, SLJIT_R3), 0);
			st_r(dst, SLJIT_R0);
			sljit_jump *done = sljit_emit_jump(C_, SLJIT_JUMP);
			sljit_label *sl = sljit_emit_label(C_);
			for (sljit_jump *j : slow) sljit_set_label(j, sl);
			call_op(d);
			stall_check();
			sljit_set_label(done, sljit_emit_label(C_));
			return true;
		}
		case 0x82: case 0xc2: case 0x8a: case 0xca: case 0x92: {   // stob stib stos stis st
			if (!memb_mode_ok(op)) return false;
			const int size = o == 0x92 ? 4 : (o == 0x8a || o == 0xca) ? 2 : 1;
			std::vector<sljit_jump *> slow;
			if (!ea(d)) return false;
			page(wpage, size, slow);
			ic_sub(2);
			ld_r(SLJIT_R0, dst);
			sljit_emit_op1(C_, size == 4 ? SLJIT_MOV32 : size == 2 ? SLJIT_MOV_U16 : SLJIT_MOV_U8, SLJIT_MEM2(SLJIT_R2, SLJIT_R3), 0, SLJIT_R0, 0);
			sljit_jump *done = sljit_emit_jump(C_, SLJIT_JUMP);
			sljit_label *sl = sljit_emit_label(C_);
			for (sljit_jump *j : slow) sljit_set_label(j, sl);
			call_op(d);
			sljit_set_label(done, sljit_emit_label(C_));
			return true;
		}
		default:
			return false;
		}
	}

	void insn(const decoded &d, uint32_t next_emitted)
	{
		const uint32_t a = d.addr, next = a + uint32_t(d.len), o = d.op >> 24;
		labels[a] = sljit_emit_label(C_);
		if (d.k == kind::bad) {   // the interpreter's (back to the loop at it)
			sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_ip, SLJIT_IMM, a);
			to_exit.push_back(sljit_emit_jump(C_, SLJIT_JUMP));
			return;
		}
		st.instructions++;
		if (auto h = spun.find(a); h != spun.end()) sljit_emit_op1(C_, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), h->second, SLJIT_IMM, 1);
		sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_pip, SLJIT_IMM, a);
		sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_MEM1(SLJIT_S0), o_ip, SLJIT_IMM, next);
		const bool may_stall = o == 0x80 || o == 0x88 || o == 0x90 || o == 0x98 || o == 0xa0 || o == 0xb0 || o == 0xc0 || o == 0xc8 ||
							   o == 0x60 || o == 0x66 || o == 0x0a;
		if (native_ok && emit_inline(d)) st.native++;
		else {
			// execute_op's case for it
			call_op(d);
			if (may_stall) stall_check();   // a stalled read: the loop retries it
		}
		// the slice's cycles
		sljit_emit_op1(C_, SLJIT_MOV_S32, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S0), o_icount);
		to_exit.push_back(sljit_emit_cmp(C_, SLJIT_SIG_LESS_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, 0));
		auto on = [&]() { if (next_emitted != next) go(next); };
		switch (d.k) {
		case kind::normal:
			if (o == 0x65 || o == 0x60 || o == 0x6e) {   // (may have taken an interrupt)
				load_ip();
				to_dispatch.push_back(sljit_emit_cmp(C_, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, next));
			}
			on();
			break;
		case kind::branch: {
			load_ip();
			auto sp = p.spins.find(a);
			if (sp != p.spins.end() && compiled(d.target)) {
				sljit_jump *no = sljit_emit_cmp(C_, SLJIT_NOT_EQUAL | SLJIT_32, SLJIT_R0, 0, SLJIT_IMM, d.target);
				// (only when the last time round all ran since the code was entered: in this slice)
				sljit_jump *fresh = sljit_emit_cmp(C_, SLJIT_EQUAL, SLJIT_MEM1(SLJIT_SP), spun.at(d.target), SLJIT_IMM, 0);
				sljit_emit_op1(C_, SLJIT_MOV, SLJIT_R0, 0, SLJIT_S0, 0);
				sljit_emit_op1(C_, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, sp->second.cost);
				sljit_emit_op1(C_, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, sljit_sw(&sp->second.loads));
				sljit_emit_icall(C_, SLJIT_CALL, SLJIT_ARGS3V(P, 32, W), SLJIT_IMM, SLJIT_FUNC_ADDR(h_spin));
				sljit_set_label(fresh, sljit_emit_label(C_));
				go(d.target);
				sljit_set_label(no, sljit_emit_label(C_));
			} else
				if_ip(d.target);
			if (d.target != next) on();
			break;
		}
		case kind::jump:
			go(d.target);
			break;
		case kind::call:
			if (d.has_target) go(d.target);
			else to_dispatch.push_back(sljit_emit_jump(C_, SLJIT_JUMP));
			break;
		case kind::ijump:
			if (auto it = p.hinted.find(a); it != p.hinted.end()) {
				load_ip();
				for (uint32_t t : it->second) if (compiled(t)) if_ip(t);
			}
			to_dispatch.push_back(sljit_emit_jump(C_, SLJIT_JUMP));
			break;
		case kind::ret:
			to_dispatch.push_back(sljit_emit_jump(C_, SLJIT_JUMP));
			break;
		default:
			break;
		}
	}

	void *build()
	{
		C_ = sljit_create_compiler(nullptr);
		if (!C_) return nullptr;
		o_ip = off(&R::ip(cpu));
		o_pip = off(&R::pip(cpu));
		o_icount = off(&R::icount(cpu));
		o_stalled = off(&R::stalled(cpu));
		o_r = off(R::regs(cpu));
		o_ac = off(&R::ac(cpu));
		rpage = rt.rpage.data();
		wpage = rt.wpage.data();
		for (auto &[br, sp] : p.spins)
			if (compiled(sp.head) && !spun.count(sp.head)) { const sljit_sw at = sljit_sw(spun.size() * sizeof(sljit_sw)); spun[sp.head] = at; }
		// entry(c, label): the spin loops' flags cleared, then straight to the instruction's code
		sljit_emit_enter(C_, 0, SLJIT_ARGS2V(P, W), 4, 2, sljit_s32(spun.size() * sizeof(sljit_sw)));
		for (auto &[h, at] : spun) sljit_emit_op1(C_, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), at, SLJIT_IMM, 0);
		sljit_emit_ijump(C_, SLJIT_JUMP, SLJIT_S1, 0);
		// every instruction, in address order (the flow from one to another as jumps)
		std::vector<uint32_t> order;
		for (auto &[a, d] : p.insns) if (p.owner.count(a)) order.push_back(a);
		for (size_t i = 0; i < order.size(); i++)
			insn(p.insns.at(order[i]), i + 1 < order.size() ? order[i + 1] : ~0u);
		// back to the loop
		sljit_label *exit = sljit_emit_label(C_);
		sljit_emit_return_void(C_);
		// IP is somewhere else: its code, if there's some
		sljit_label *dispatch = sljit_emit_label(C_);
		sljit_emit_op1(C_, SLJIT_MOV, SLJIT_R0, 0, SLJIT_S0, 0);
		sljit_emit_icall(C_, SLJIT_CALL, SLJIT_ARGS1(W, P), SLJIT_IMM, SLJIT_FUNC_ADDR(h_label));
		sljit_jump *none = sljit_emit_cmp(C_, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);
		sljit_emit_ijump(C_, SLJIT_JUMP, SLJIT_R0, 0);
		sljit_set_label(none, exit);
		for (sljit_jump *j : to_exit) sljit_set_label(j, exit);
		for (sljit_jump *j : to_dispatch) sljit_set_label(j, dispatch);
		for (auto &[j, t] : to_label) sljit_set_label(j, labels.at(t));
		void *code = sljit_generate_code(C_, 0, nullptr);
		if (!code) { sljit_free_compiler(C_); return nullptr; }
		st.code_bytes = sljit_get_generated_code_size(C_);
		rt.nlabel.assign(runtime::LIMIT / 4, 0);
		for (auto &[a, l] : labels)
			if (a < runtime::LIMIT && p.insns.at(a).k != kind::bad) rt.nlabel[a >> 2] = sljit_get_label_addr(l);
		sljit_free_compiler(C_);
		return code;
	}
};

} // namespace

bool build_native(runtime &rt, uint32_t (*rd)(void *, uint32_t), void *ctx, uint32_t limit, const hints &map, native_stats *stats)
{
	const auto t0 = std::chrono::steady_clock::now();
	if (!rt.cpu) return false;
	native_stats st;
	auto prog = std::make_shared<program>(analyse([&](uint32_t a) { return rd(ctx, a); }, limit, map));
	gen g{ nullptr, *prog, rt, *rt.cpu, st };
	void *code = g.build();
	if (!code) return false;
	if (rt.native_code) sljit_free_code(rt.native_code, nullptr);
	rt.native_code = code;
	rt.native_program = prog;
	rt.nentry = reinterpret_cast<runtime::nfn_t>(code);
	rt.rd = rd;
	rt.rd_ctx = ctx;
	rt.map = map;
	rt.limit = limit;
	st.functions = prog->functions.size();
	st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
	rt.st.functions = st.functions;
	rt.st.instructions = st.instructions;
	rt.st.dropped = 0;
	if (stats) *stats = st;
	return true;
}

void free_native(void *code)
{
	if (code) sljit_free_code(code, nullptr);
}

} // namespace i960rec
