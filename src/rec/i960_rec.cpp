#include "i960_rec.h"
#include "i960_ops.h"
#include "i960_decode.h"
#include "i960_native.h"

namespace i960rec {

using R = i960_rec;

void runtime::stop(i960_device &) {}

void free_native(void *code);   // (i960_native.cpp)
runtime::~runtime() { free_native(native_code); }

void dispatch(i960_device &c)
{
	const fn_t f = c.rec()->lookup(R::ip(c));
#if defined(__clang__)
	if (f) [[clang::musttail]] return f(c);   // (the C++ build's: clang only)
#else
	if (f) return f(c);
#endif
}

void runtime::install(const gen_fn *f, size_t nf, const gen_insn *in, size_t ni, uint32_t (*read)(void *, uint32_t), void *ctx)
{
	rd = read;
	rd_ctx = ctx;
	table.assign(LIMIT / 4, nullptr);
	fns.assign(nf, stop);
	code.resize(nf);
	span.assign(nf, { 0, 0 });
	for (size_t k = 0; k < nf; k++) code[k] = f[k].code;
	insns.assign(in, in + ni);
	std::stable_sort(insns.begin(), insns.end(), [](const gen_insn &a, const gen_insn &b) { return a.fn < b.fn; });
	for (size_t i = 0; i < insns.size(); i++) {
		auto &s = span[insns[i].fn];
		if (!s.second) s.first = uint32_t(i);
		s.second++;
	}
	st = {};
	for (size_t k = 0; k < nf; k++) enable(k, matches(k));
	recount();
}

bool runtime::matches(size_t k) const
{
	for (uint32_t i = span[k].first; i < span[k].first + span[k].second; i++) {
		const gen_insn &x = insns[i];
		if (rd(rd_ctx, x.addr) != x.word || (length(x.word) == 8 && rd(rd_ctx, x.addr + 4) != x.disp)) return false;
	}
	return true;
}

void runtime::enable(size_t k, bool on)
{
	fns[k] = on ? code[k] : stop;
	for (uint32_t i = span[k].first; i < span[k].first + span[k].second; i++) {
		const gen_insn &x = insns[i];
		if (x.addr < LIMIT) table[x.addr >> 2] = on ? code[k] : nullptr;
	}
}

void runtime::recount()
{
	st.functions = st.instructions = st.dropped = 0;
	for (size_t j = 0; j < fns.size(); j++) {
		if (fns[j] != stop) { st.functions++; st.instructions += span[j].second; }
		else st.dropped += span[j].second;
	}
}

void runtime::code_changed(uint32_t addr, uint32_t n)
{
	if (nentry) {   // native: made again from the program as it is now (a few tens of ms), if it changed
		bool hit = false;
		for (auto &[a, d] : native_program->insns)
			if (a < addr + n && addr < a + uint32_t(d.len) && (rd(rd_ctx, a) != d.op || (d.len == 8 && rd(rd_ctx, a + 4) != d.disp))) { hit = true; break; }
		if (hit) {
			hints m = map;
			if (!build_native(*this, rd, rd_ctx, limit, m)) { nentry = nullptr; table.assign(LIMIT / 4, nullptr); }
		}
		return;
	}
	for (size_t k = 0; k < fns.size(); k++) {
		bool hit = false;
		for (uint32_t i = span[k].first; i < span[k].first + span[k].second && !hit; i++) {
			const gen_insn &x = insns[i];
			hit = x.addr < addr + n && addr < x.addr + uint32_t(length(x.word));
		}
		if (hit) enable(k, matches(k));
	}
	recount();
}

// i960_cpu_device::execute_run, with the recompiled functions for the code they cover
void runtime::execute(i960_device &c)
{
	if (!R::burst_mode(c)) R::check_immediate_irqs(c);
	int &ic = R::icount(c);
	const bool hooked = bool(c.on_instruction);   // (the instruction hook: the interpreter only)
	while (ic > 0) {
		if (!hooked && !R::burst_mode(c)) {
			const uint32_t ip = R::ip(c);
			if (nentry) {
				const uintptr_t l = ip < LIMIT && !(ip & 3) ? nlabel[ip >> 2] : 0;
				if (l) {
					R::stalled(c) = false;
					const int before = ic;
					nentry(&c, l);
					if (ic != before || R::ip(c) != ip) continue;
				}
			} else if (const fn_t f = lookup(ip)) {
				R::stalled(c) = false;
				const int before = ic;
				f(c);
				if (ic != before || R::ip(c) != ip) continue;
				// (a function that didn't take its entry: the interpreter, so that it moves on)
			}
		}
		if (count_interp) {
			st.interpreted++;
			const uint32_t ip = R::ip(c);
			if (ip < LIMIT) {
				if (interp_pc.empty()) interp_pc.assign(LIMIT / 4, 0);
				interp_pc[ip >> 2]++;
			}
		}
		R::interp_one(c);
	}
}

} // namespace i960rec
