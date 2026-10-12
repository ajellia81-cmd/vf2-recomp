// Checks the player's vf2.zip against MAME's set and prints a checklist; with an output
// directory, also writes each region as <name>.bin (ROM data: keep it under build/).
//   vf2romset roms/vf2.zip [outdir]
#include "../src/host/romset.h"
#include <cstdio>
#include <filesystem>

int main(int argc, char **argv)
{
	if (argc < 2) { fprintf(stderr, "usage: vf2romset <vf2.zip> [outdir]\n"); return 2; }
	romset::result r = romset::load(argv[1]);
	if (r.roms.empty()) { fprintf(stderr, "%s\n", r.error.c_str()); return 1; }
	static const char *state[] = { "ok", "MISSING", "WRONG VERSION", "DAMAGED" };
	int prev = -1;
	for (const auto &s : r.roms) {
		if (s.region != prev) printf("%s (%zx bytes)\n", romset::region_name(s.region), r.regions[s.region].size());
		prev = s.region;
		printf("  [%s] %-14s crc %08x at %07x  %s\n", s.state == s.OK ? "x" : " ", s.name.c_str(), s.crc, s.offset, state[s.state]);
	}
	printf("%zu ROMs: %s\n", r.roms.size(), r.ok ? "complete" : r.error.c_str());
	if (r.ok && argc > 2) {
		std::filesystem::create_directories(argv[2]);
		for (int i = 0; i < romset::N_REGIONS; i++) {
			std::string path = std::string(argv[2]) + "/" + romset::region_name(i) + ".bin";
			FILE *f = fopen(path.c_str(), "wb");
			if (!f) { perror(path.c_str()); return 1; }
			fwrite(r.regions[i].data(), 1, r.regions[i].size(), f);
			fclose(f);
		}
		printf("regions written to %s\n", argv[2]);
	}
	return r.ok ? 0 : 1;
}
