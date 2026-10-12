// i960 disassembly of VF2's program (MAME's disassembler), straight from the zip.
//   vf2dis roms/vf2.zip <hex address> [count]
#include "../src/host/romset.h"
#include "emu.h"
#include "../third_party/mame/i960/i960dis.h"
#include <sstream>

int main(int argc, char **argv)
{
	if (argc < 3) { fprintf(stderr, "usage: vf2dis <vf2.zip> <hex address> [count]\n"); return 2; }
	romset::result r = romset::load(argv[1]);
	if (!r.ok) { fprintf(stderr, "%s\n", r.error.c_str()); return 1; }
	const auto &rom = r.regions[romset::MAINCPU];
	uint32_t pc = strtoul(argv[2], nullptr, 16);
	int n = argc > 3 ? atoi(argv[3]) : 32;
	i960_disassembler dis;
	util::disasm_interface::data_buffer buf;
	buf.rd = [&](offs_t a) -> u8 { return a < rom.size() ? rom[a] : 0; };
	for (int i = 0; i < n; i++) {
		std::ostringstream s;
		offs_t len = dis.disassemble(s, pc, buf, buf) & util::disasm_interface::LENGTHMASK;
		printf("%08x: %08x %-9s %s\n", pc, buf.r32(pc), len > 4 ? string_format("%08x", buf.r32(pc + 4)).c_str() : "", s.str().c_str());
		pc += len;
	}
}
