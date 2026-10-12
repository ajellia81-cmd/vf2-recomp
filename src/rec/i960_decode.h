// The i960KB's instructions as the recompiler sees them: length, what they do to the flow of
// control, and whether MAME's core (third_party/mame/i960/i960.cpp, execute_op) implements them.
// Shared by the analysis, the code generators and the coverage runs.
#pragma once
#include <cstdint>

namespace i960rec {

enum class kind : uint8_t {
	normal,     // on to the next instruction
	branch,     // a conditional branch to a static target, or on
	jump,       // b, or bx to a static address: never on
	ijump,      // bx through a register: the target is known only at run time
	call,       // call / callx / calls / bal / balx: to a function, back to the return site (usually next)
	ret,        // ret
	bad,        // not an instruction MAME's core runs (its fatalerror)
};

// MEMB's addressing modes that take a displacement word after the instruction
inline bool memb_has_disp(uint32_t op) { return (op & 0x1000) && ((((op >> 10) & 0xf) == 5) || ((op >> 10) & 0xf) >= 0xc); }

inline bool is_mem(uint32_t op) { return (op >> 24) >= 0x80; }
inline int length(uint32_t op) { return is_mem(op) && memb_has_disp(op) ? 8 : 4; }

// The MEM format's effective address, when it doesn't depend on registers (MEMA's absolute
// offset; MEMB's IP-relative and absolute displacements). Returns false otherwise.
inline bool static_ea(uint32_t addr, uint32_t op, uint32_t disp, uint32_t &ea)
{
	if (!(op & 0x1000)) {
		if (op & 0x2000) return false;
		ea = op & 0xfff;   // (MAME: opcode & 0x1fff with bit 12 clear)
		return true;
	}
	switch ((op >> 10) & 0xf) {
	case 0x5: ea = disp + addr + 8; return true;
	case 0xc: ea = disp; return true;
	default: return false;
	}
}

// MAME's get_ea modes: 4, 5, 7, c-f for MEMB (others are fatal)
inline bool memb_mode_ok(uint32_t op)
{
	if (!(op & 0x1000)) return true;
	const int m = (op >> 10) & 0xf;
	return m == 4 || m == 5 || m == 7 || m >= 0xc;
}

// Whether execute_op has a case for the instruction (else it calls fatalerror)
inline bool implemented(uint32_t op)
{
	const uint32_t o = op >> 24, s = (op >> 7) & 0xf;
	switch (o) {
	case 0x08: case 0x09: case 0x0a: case 0x0b:
		return true;
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e:
		return true;
	case 0x58: return s != 0x5;
	case 0x59: return s <= 3 || s == 8 || (s >= 0xa && s != 0xf);
	case 0x5a: return s <= 7 || s == 0xc || s == 0xe;
	case 0x5b: return s == 0 || s == 2;
	case 0x5c: case 0x5d: case 0x5e: case 0x5f: return s == 0xc;
	case 0x60: return s == 0 || s == 2;
	case 0x64: return s == 0 || s == 1 || s == 4 || s == 5;
	case 0x65: return s == 5;
	case 0x66: return s == 0 || s == 0xd;
	case 0x67: return s <= 1 || (s >= 4 && s <= 7);
	case 0x68: return s <= 3 || s == 5 || (s >= 8 && s != 0xf);
	case 0x69: return s == 0 || s == 2 || s == 5 || (s >= 8 && s != 0xf);
	case 0x6c: return s <= 3 || s == 9;
	case 0x6d: return s == 9;
	case 0x6e: return s == 1 || s == 2 || s == 9;
	case 0x70: return s == 1 || s == 8 || s == 0xb;
	case 0x74: return s == 1 || s == 8 || s == 9 || s == 0xb;
	case 0x78: return s == 0xb || s == 0xc || s == 0xd || s == 0xf;
	case 0x79: return s == 0xb || s == 0xc || s == 0xd || s == 0xf;
	case 0x80: case 0x82: case 0x84: case 0x85: case 0x86: case 0x88: case 0x8a: case 0x8c:
	case 0x90: case 0x92: case 0x98: case 0x9a: case 0xa0: case 0xa2: case 0xb0: case 0xb2:
	case 0xc0: case 0xc2: case 0xc8: case 0xca:
		return memb_mode_ok(op);
	default:
		return false;
	}
}

inline int32_t sext(uint32_t v, int bits) { return int32_t(v << (32 - bits)) >> (32 - bits); }

struct decoded
{
	uint32_t addr = 0, op = 0, disp = 0;
	int len = 4;
	kind k = kind::normal;
	uint32_t target = 0;      // static target (branch, jump, call when has_target)
	bool has_target = false;
};

// rd(a): the 32-bit word at a
template <typename Rd> decoded decode(uint32_t addr, Rd rd)
{
	decoded d;
	d.addr = addr;
	d.op = rd(addr);
	d.len = length(d.op);
	if (d.len == 8) d.disp = rd(addr + 4);
	const uint32_t op = d.op, o = op >> 24;
	if (!implemented(op)) { d.k = kind::bad; return d; }
	const uint32_t ctrl_target = addr + 4 + uint32_t(sext(op, 24) - 4);   // MAME: IP + get_disp
	const uint32_t cobr_target = addr + 4 + uint32_t(sext(op, 13) - 4);   // IP + get_disp_s
	switch (o) {
	case 0x08: d.k = kind::jump; d.target = ctrl_target; d.has_target = true; break;
	case 0x09: d.k = kind::call; d.target = ctrl_target; d.has_target = true; break;
	case 0x0a: d.k = kind::ret; break;
	case 0x0b: d.k = kind::call; d.target = ctrl_target; d.has_target = true; break;   // bal
	case 0x10: case 0x18:   // bno, faultno (MAME branches on it)
		d.k = kind::branch; d.target = ctrl_target; d.has_target = true; break;
	case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
		d.k = kind::branch; d.target = ctrl_target & ~3u; d.has_target = true; break;
	case 0x30: case 0x37:   // bbc, bbs: no alignment
		d.k = kind::branch; d.target = cobr_target; d.has_target = true; break;
	case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36:
	case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e:
		d.k = kind::branch; d.target = cobr_target & ~3u; d.has_target = true; break;
	case 0x84: case 0x85: case 0x86: {   // bx, balx, callx
		uint32_t ea;
		const bool st = static_ea(addr, op, d.disp, ea);
		if (o == 0x84) d.k = st ? kind::jump : kind::ijump;
		else d.k = kind::call;
		if (st) { d.target = ea; d.has_target = true; }
		break;
	}
	case 0x66:
		if (((op >> 7) & 0xf) == 0) d.k = kind::call;   // calls: through the system procedure table
		break;
	default: break;
	}
	return d;
}

} // namespace i960rec
