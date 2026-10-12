// Lists VF2's `balx <thunk>, r14` call sites: the thunks read data placed inline after the call
// (text for the screen, mostly) and return past it, so the code resumes somewhere a lifter can't
// see from the instruction stream. For each site: the thunk and where it returns.
//   vf2thunks roms/vf2.zip [observed.txt]
// observed.txt (from `vf2run --thunks`) adds the return sites seen at run time and checks them.
// The output is derived from the ROM: keep it under build/.
#include "../src/host/romset.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

int main(int argc, char **argv)
{
	if (argc < 2) { fprintf(stderr, "usage: vf2thunks <vf2.zip> [observed.txt]\n"); return 2; }
	romset::result r = romset::load(argv[1]);
	if (!r.ok) { fprintf(stderr, "%s\n", r.error.c_str()); return 1; }
	const auto &rom = r.regions[romset::MAINCPU];
	auto rd = [&](uint32_t a) -> uint32_t { uint32_t v = 0; if (a + 4 <= rom.size()) memcpy(&v, &rom[a], 4); return v; };
	// The thunks (and the 0x93c4-0x9408 entries that convert g0 first and fall into 0x9448):
	//  0x9444 family: skip dwords until one whose top byte is 0, then bx past it
	//  0x4ec00: the data is 8 bytes, bx 0x8(r14)
	//  0x502a4: the callee at 0x502c0 parses the data; the return is only known at run time
	std::set<uint32_t> skip_family = { 0x9444 };
	for (uint32_t a = 0x93c4; a <= 0x9408; a += 4) skip_family.insert(a);
	std::map<uint32_t, std::set<uint32_t>> observed;   // site -> returns seen
	if (argc > 2) {
		FILE *f = fopen(argv[2], "r");
		if (!f) { perror(argv[2]); return 1; }
		unsigned s, t, ret;
		while (fscanf(f, "%x %x %x", &s, &t, &ret) == 3) observed[s].insert(ret);
		fclose(f);
	}
	int n = 0, dynamic = 0, mismatches = 0;
	std::map<uint32_t, int> per_thunk;
	for (uint32_t pc = 0; pc + 8 <= rom.size(); pc += 4) {
		uint32_t op = rd(pc);
		if ((op >> 24) != 0x85 || !(op & 0x1000)) continue;         // balx, MEMB
		if (((op >> 10) & 0xf) != 0xc || ((op >> 19) & 0x1f) != 14) continue;   // absolute, link in r14
		uint32_t target = rd(pc + 4);
		uint32_t link = pc + 8;
		long ret = -1;
		if (skip_family.count(target)) {
			uint32_t a = link;
			while (a + 4 <= rom.size() && rd(a) > 0xffffff) a += 4;
			ret = a + 4;
		} else if (target == 0x4ec00) {
			ret = link + 8;
		} else if (target != 0x502a4) {
			continue;
		}
		n++;
		per_thunk[target]++;
		std::string seen;
		if (auto it = observed.find(pc); it != observed.end())
			for (uint32_t o : it->second) {
				char b[64]; snprintf(b, sizeof b, "%s%06x", seen.empty() ? " (seen at run time: " : ", ", o); seen += b;
				if (ret >= 0 && o != uint32_t(ret)) mismatches++;
			}
		if (!seen.empty()) seen += ")";
		if (ret < 0) dynamic++;
		if (ret >= 0) printf("%06x balx %06x -> returns to %06x%s\n", pc, target, uint32_t(ret), seen.c_str());
		else printf("%06x balx %06x -> returns where the callee says%s\n", pc, target, seen.c_str());
	}
	fprintf(stderr, "%d call sites:", n);
	for (auto &[t, c] : per_thunk) fprintf(stderr, " %06x x%d", t, c);
	fprintf(stderr, "; %d returns only known at run time", dynamic);
	if (!observed.empty()) fprintf(stderr, "; %zu sites seen running, %d disagree with the static return", observed.size(), mismatches);
	fprintf(stderr, "\n");
}
