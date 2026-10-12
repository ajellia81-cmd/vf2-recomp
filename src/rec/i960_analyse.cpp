#include "i960_analyse.h"
#include <algorithm>
#include <cstdio>
#include <deque>

namespace i960rec {

static void parse_line(hints &h, const char *line)
{
	unsigned a, b;
	unsigned long long n;
	if (sscanf(line, "pc %x %llu", &a, &n) == 2) h.covered[a] += n;
	else if (sscanf(line, "edge %x %x", &a, &b) == 2) h.edges[a].insert(b);
	else if (sscanf(line, "thunk %x", &a) == 1) h.thunks.insert(a);
	else if (sscanf(line, "entry %x", &a) == 1) h.entries.insert(a);
}

bool hints::read(const std::string &path)
{
	FILE *f = fopen(path.c_str(), "r");
	if (!f) return false;
	char line[256];
	while (fgets(line, sizeof line, f)) parse_line(*this, line);
	fclose(f);
	return true;
}

void hints::parse(const char *text)
{
	std::string line;
	for (const char *p = text; *p; p++) {
		if (*p == '\n') { parse_line(*this, line.c_str()); line.clear(); }
		else line += *p;
	}
	if (!line.empty()) parse_line(*this, line.c_str());
}

namespace {

struct analyser
{
	const std::function<uint32_t(uint32_t)> &rd;
	uint32_t limit;
	const hints &h;
	program p;
	std::set<uint32_t> roots;   // function entries

	bool in_rom(uint32_t a) const { return a < limit && !(a & 3); }

	const decoded *get(uint32_t a)
	{
		auto it = p.insns.find(a);
		if (it != p.insns.end()) return &it->second;
		if (!in_rom(a) || a + 4 > limit) return nullptr;
		decoded d = decode(a, [&](uint32_t x) { return x + 4 <= limit ? rd(x) : 0u; });
		if (d.len == 8 && a + 8 > limit) d.k = kind::bad;
		return &(p.insns[a] = d);
	}

	// does the flow go on to the next instruction?
	bool falls(const decoded &d) const
	{
		switch (d.k) {
		case kind::jump: case kind::ijump: case kind::ret: case kind::bad: return false;
		case kind::call: return !(d.has_target && h.thunks.count(d.target));
		default: return true;
		}
	}

	// where the flow may go from d, other than to a function it calls
	template <typename F> void successors(const decoded &d, F &&f) const
	{
		if (falls(d)) f(d.addr + uint32_t(d.len));
		if ((d.k == kind::branch || d.k == kind::jump) && d.has_target) f(d.target);
	}

	void discover()
	{
		std::deque<uint32_t> work;
		auto push = [&](uint32_t a) { if (in_rom(a) && !p.insns.count(a)) work.push_back(a); };
		auto root = [&](uint32_t a) { if (in_rom(a)) { roots.insert(a); push(a); } };
		root(rd(12));   // the reset IP
		for (uint32_t a : h.entries) root(a);
		for (auto &[a, n] : h.covered) push(a);
		for (auto &[a, ts] : h.edges) for (uint32_t t : ts) push(t);
		while (!work.empty()) {
			const uint32_t a = work.front();
			work.pop_front();
			if (p.insns.count(a)) continue;
			const decoded *d = get(a);
			if (!d) continue;
			const decoded dd = *d;
			if (dd.k == kind::call && dd.has_target) root(dd.target);
			successors(dd, push);
			if (auto it = h.edges.find(a); it != h.edges.end())
				for (uint32_t t : it->second) {
					if (dd.k == kind::call) root(t);
					else push(t);
				}
		}
		// the indirect jumps' targets (only those: a ret's go back to call sites, and an edge from
		// a plain instruction is an interrupt)
		for (auto &[a, ts] : h.edges) {
			auto it = p.insns.find(a);
			if (it == p.insns.end()) continue;
			const decoded &d = it->second;
			if (d.k == kind::ijump) p.hinted[a].assign(ts.begin(), ts.end());
			else if (d.k != kind::call && d.k != kind::ret)
				for (uint32_t t : ts) if (!(d.has_target && t == d.target) && t != a + uint32_t(d.len)) roots.insert(t);   // interrupt handlers
		}
		p.thunks = h.thunks;
	}

	// claim what's reachable from `entry` without leaving through calls, returns or indirect jumps
	void claim(uint32_t entry, int index, std::vector<uint32_t> &body)
	{
		std::deque<uint32_t> work{ entry };
		while (!work.empty()) {
			const uint32_t a = work.front();
			work.pop_front();
			auto it = p.insns.find(a);
			if (it == p.insns.end() || p.owner.count(a)) continue;
			p.owner[a] = index;
			body.push_back(a);
			if (it->second.k == kind::bad) continue;
			successors(it->second, [&](uint32_t t) { work.push_back(t); });
		}
	}

	void partition()
	{
		auto make = [&](uint32_t e) {
			if (!p.insns.count(e) || p.owner.count(e)) return;
			function fn;
			fn.entry = e;
			claim(e, int(p.functions.size()), fn.body);
			std::sort(fn.body.begin(), fn.body.end());
			p.functions.push_back(std::move(fn));
		};
		for (uint32_t e : roots) make(e);
		// then the indirect jumps' targets that no function reached, and whatever else was found
		for (auto &[a, ts] : p.hinted) for (uint32_t t : ts) make(t);
		std::vector<uint32_t> rest;
		for (auto &[a, d] : p.insns) if (!p.owner.count(a)) rest.push_back(a);
		for (uint32_t a : rest) make(a);
	}
};

} // namespace

// cycles of the instructions a spin loop may hold (execute_op's counts), 0 for any other
static uint32_t spin_cost(const decoded &d)
{
	const uint32_t o = d.op >> 24;
	switch (o) {
	case 0x80: case 0x88: case 0x90: case 0xc0: case 0xc8: {   // ldob ldos ld ldib ldis, fixed address
		uint32_t ea;
		return static_ea(d.addr, d.op, d.disp, ea) ? 4 : 0;
	}
	case 0x5a: {   // cmpo, cmpi
		const uint32_t s = (d.op >> 7) & 0xf;
		return s <= 1 ? 1 : 0;
	}
	case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: return 1;
	case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36:
	case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: return 4;
	default: return 0;
	}
}

static void find_spins(program &p)
{
	for (auto &[a, d] : p.insns) {
		if (d.k != kind::branch || !d.has_target || d.target >= a || a - d.target > 64) continue;
		if (!spin_cost(d)) continue;
		uint32_t cost = 0;
		std::vector<uint32_t> loads;
		bool ok = true;
		for (uint32_t x = d.target; x < a && ok;) {
			auto it = p.insns.find(x);
			if (it == p.insns.end() || it->second.k != kind::normal || !spin_cost(it->second) || p.owner_of(x) != p.owner_of(a)) { ok = false; break; }
			cost += spin_cost(it->second);
			if ((it->second.op >> 24) >= 0x80) { uint32_t ea = 0; static_ea(x, it->second.op, it->second.disp, ea); loads.push_back(ea); }
			x += uint32_t(it->second.len);
		}
		if (ok && !loads.empty()) p.spins[a] = { d.target, cost + spin_cost(d), loads };
	}
}

program analyse(const std::function<uint32_t(uint32_t)> &rd, uint32_t limit, const hints &h)
{
	analyser an{ rd, limit, h };
	an.discover();
	an.partition();
	find_spins(an.p);
	return std::move(an.p);
}

void write_map(const program &p, const hints &h, const char *path)
{
	FILE *f = fopen(path, "w");
	if (!f) return;
	const bool cpp = std::string(path).size() > 4 && std::string(path).substr(std::string(path).size() - 4) == ".cpp";
	if (cpp) fprintf(f, "// The address map (made by tools/recomp/vf2recomp map): addresses only.\n#include \"i960_analyse.h\"\nconst char i960rec::vf2_map_text[] = R\"MAP(");
	fprintf(f, "# Virtua Fighter 2 (Model 2A) i960 code map for the recompiler (src/rec/i960_analyse.h): where\n"
			   "# the functions start, where the indirect jumps and calls lead, and the calls that return past\n"
			   "# data. Addresses only, nothing from the ROM. Made by tools/recomp/vf2recomp from vf2run --cov runs.\n");
	for (uint32_t t : p.thunks) fprintf(f, "thunk %06x\n", t);
	for (const function &fn : p.functions) fprintf(f, "entry %06x\n", fn.entry);
	for (auto &[a, ts] : h.edges) {
		auto it = p.insns.find(a);
		if (it == p.insns.end()) continue;
		const kind k = it->second.k;
		if (k != kind::ijump && !(k == kind::call && !it->second.has_target)) continue;
		for (uint32_t t : ts) fprintf(f, "edge %06x %06x\n", a, t);
	}
	if (cpp) fprintf(f, ")MAP\";\n");
	fclose(f);
}

} // namespace i960rec
