// Lockstep check of the recompiled i960 code: two boards, the same inputs, one on MAME's
// interpreter and one on the recompiled code, compared as they run.
//   vf2lock <vf2.zip> <frames> [options]
//     --cpp            the build's recompiled C++ (build/gen) instead of the native code made at
//                      start-up (the default, with the build's address map)
//     --map file       the native code made with this address map
//     --script file    inputs (src/host/input_script.h)
//     --version N      the game assignment's version (0 Original, 1 Version 2.1), set as vf2run does
//     --ko P@a[-b]     as vf2run's (knock player P out in every round from frame a)
//     --sound          run the sound board on both
//     --byte-link      the host's link to the sound board (longer slices: the game's timing)
//     --slices         compare the CPU after every slice the scheduler gives it, not every frame
//     --pictures N     compare the pictures every N frames (default 60; 0: never)
//     --every N        report every N frames (default 600)
//     --culling N      the scenery culling (patches the program: the recompiled code follows it)
//     --warning        shorten the WARNING screen at frame 2 (a patch while it runs)
// Every frame: the i960's whole state (registers, register cache, interrupt and stall state,
// cycle count), its clock and the TGP's, work RAM, RAM at 0x200000, buffer RAM, backup RAM,
// tile, palette and colour tables, the TGP's FIFOs (through the clocks). Stops at the first
// difference, with what differs.
#include "../src/machine/model2a.h"
#include "../src/host/input_script.h"
#include "../src/rec/i960_native.h"
#include "../src/rec/i960_analyse.h"
#include <chrono>
#include <cstring>

static uint64_t fnv(const void *p, size_t n, uint64_t h = 1469598103934665603ull)
{
	const uint8_t *b = static_cast<const uint8_t *>(p);
	for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 1099511628211ull;
	return h;
}

struct slice_rec { uint64_t now; uint64_t hash; u32 ip, pip; };

int main(int argc, char **argv)
{
	if (argc < 3) { fprintf(stderr, "usage: vf2lock <vf2.zip> <frames> [--cpp] [--map f] [--script f] [--version N] [--ko P@a] [--slices]\n"); return 2; }
	const uint64_t frames = strtoull(argv[2], nullptr, 10);
	std::string script_path, map_path;
	bool native = true, slices = false, sound = false, warning = false, byte_link = false;
	int version = -1, pictures = 60, every = 600, culling = -1;
	struct knockout { int player; uint64_t from, to; };
	std::vector<knockout> kos;
	for (int i = 3; i < argc; i++) {
		std::string a = argv[i];
		auto next = [&]() -> const char * { if (i + 1 >= argc) { fprintf(stderr, "%s needs a value\n", a.c_str()); exit(2); } return argv[++i]; };
		if (a == "--cpp") native = false;
		else if (a == "--map") map_path = next();
		else if (a == "--script") script_path = next();
		else if (a == "--version") version = atoi(next());
		else if (a == "--slices") slices = true;
		else if (a == "--sound") sound = true;
		else if (a == "--byte-link") byte_link = true;
		else if (a == "--warning") warning = true;
		else if (a == "--pictures") pictures = atoi(next());
		else if (a == "--every") every = atoi(next());
		else if (a == "--culling") culling = atoi(next());
		else if (a == "--ko") {
			const char *v = next();
			char *e;
			knockout k{ int(strtol(v, &e, 10)), 0, UINT64_MAX };
			if (*e == '@') { k.from = strtoull(e + 1, &e, 10); if (*e == '-') k.to = strtoull(e + 1, &e, 10); }
			kos.push_back(k);
		}
		else { fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
	}
	input_script script;
	if (!script_path.empty()) {
		std::string err;
		if (!script.load(script_path, err)) { fprintf(stderr, "%s\n", err.c_str()); return 2; }
	}
	model2a a(romset::load(argv[1])), b(romset::load(argv[1]));
	a.use_interpreter();
	for (model2a *m : { &a, &b }) {
		m->set_sound(sound);
		m->set_byte_link(byte_link);
		m->set_draw_3d(pictures > 0);
		if (culling >= 0 && !m->set_scenery_culling(culling)) { fprintf(stderr, "--culling: not the program known\n"); return 1; }
		m->reset();
	}
	auto t0 = std::chrono::steady_clock::now();
	if (native) {
		i960rec::native_stats st;
		if (!b.use_native(map_path, &st)) { fprintf(stderr, "native recompile failed\n"); return 1; }
		printf("native: %zu functions, %zu instructions (%zu as native code), %zu KB of code in %.0f ms\n",
			   st.functions, st.instructions, st.native, st.code_bytes / 1024, st.ms);
	} else if (!b.use_generated()) { fprintf(stderr, "this build has no recompiled C++ (make rec)\n"); return 1; }
	const i960rec::stats &rs = b.recompiled()->st;
	printf("recompiled: %zu functions, %zu instructions (%zu dropped: the program differs)\n", rs.functions, rs.instructions, rs.dropped);
	b.recompiled()->count_interp = true;
	if (version >= 0) {
		for (model2a *m : { &a, &b }) {
			for (int i = 0; i < 120 && m->assignment(model2a::game_assignment::version) < 0; i++) m->run_frame();
			m->set_assignment(model2a::game_assignment::version, version);
			m->reset();
		}
		printf("version %d\n", a.assignment(model2a::game_assignment::version));
	}
	a.cpu().time_runs = b.cpu().time_runs = true;
	std::vector<slice_rec> sa, sb;
	std::vector<u32> wa, wb;
	auto hook = [&](model2a &m, std::vector<slice_rec> &v) {
		m.cpu().on_slice = [&m, &v]() {
			std::vector<u32> w;
			m.cpu().state_words(w);
			v.push_back({ m.now(), fnv(w.data(), w.size() * 4), m.cpu().ip(), m.cpu().pip() });
		};
	};
	if (slices) { hook(a, sa); hook(b, sb); }
	double ta = 0, tb = 0;
	uint64_t last_interp = 0;
	for (uint64_t f = 0; f < frames; f++) {
		if (warning && f == 2) { a.shorten_warning(); b.shorten_warning(); }
		auto c0 = std::chrono::steady_clock::now();
		a.run_frame();
		auto c1 = std::chrono::steady_clock::now();
		b.run_frame();
		auto c2 = std::chrono::steady_clock::now();
		ta += std::chrono::duration<double>(c1 - c0).count();
		tb += std::chrono::duration<double>(c2 - c1).count();
		bool bad = false;
		if (slices) {
			const size_t n = std::min(sa.size(), sb.size());
			for (size_t i = 0; i < n && !bad; i++)
				if (sa[i].now != sb[i].now || sa[i].hash != sb[i].hash) {
					printf("frame %llu slice %zu differs: interp now %llu ip %08x pip %08x, rec now %llu ip %08x pip %08x\n", (unsigned long long)f, i,
						   (unsigned long long)sa[i].now, sa[i].ip, sa[i].pip, (unsigned long long)sb[i].now, sb[i].ip, sb[i].pip);
					if (i) printf("  slice before: ip %08x pip %08x\n", sa[i - 1].ip, sa[i - 1].pip);
					bad = true;
				}
			if (!bad && sa.size() != sb.size()) { printf("frame %llu: %zu slices against %zu\n", (unsigned long long)f, sa.size(), sb.size()); bad = true; }
			sa.clear();
			sb.clear();
		}
		a.cpu().state_words(wa);
		b.cpu().state_words(wb);
		if (wa != wb) {
			printf("frame %llu: CPU state differs\n", (unsigned long long)f);
			for (size_t i = 0; i < wa.size(); i++) if (wa[i] != wb[i]) printf("  word %zu: interp %08x rec %08x\n", i, wa[i], wb[i]);
			bad = true;
		}
		if (a.now() != b.now() || a.tgp_now() != b.tgp_now()) { printf("frame %llu: clocks differ (i960 %llu/%llu, TGP %llu/%llu)\n", (unsigned long long)f,
			(unsigned long long)a.now(), (unsigned long long)b.now(), (unsigned long long)a.tgp_now(), (unsigned long long)b.tgp_now()); bad = true; }
		auto mem = [&](const char *what, const std::vector<u8> &x, const std::vector<u8> &y) {
			if (x != y) {
				size_t i = 0;
				while (i < x.size() && x[i] == y[i]) i++;
				printf("frame %llu: %s differs (first at +%zx)\n", (unsigned long long)f, what, i);
				bad = true;
			}
		};
		mem("work RAM", a.workram(), b.workram());
		mem("backup RAM", a.backup_ram(), b.backup_ram());
		auto region = [&](const char *what, u32 from, u32 n) {
			for (u32 x = from; x < from + n; x += 4)
				if (a.peek32(x) != b.peek32(x)) { printf("frame %llu: %s differs at %08x\n", (unsigned long long)f, what, x); bad = true; return; }
		};
		region("RAM 0x200000", 0x200000, 0x40000);
		region("buffer RAM", 0x900000, 0x20000);
		region("palette", 0x1800000, 0x4000);
		region("colour tables", 0x1810000, 0xc000);
		if (fnv(a.tiles().tile_ram.data(), a.tiles().tile_ram.size() * 2) != fnv(b.tiles().tile_ram.data(), b.tiles().tile_ram.size() * 2)) { printf("frame %llu: tile RAM differs\n", (unsigned long long)f); bad = true; }
		if (pictures > 0 && f % pictures == 0) {
			const bitmap_rgb32 &pa = a.render(true);
			std::vector<u32> va(pa.data(), pa.data() + size_t(pa.width()) * pa.height());
			const bitmap_rgb32 &pb = b.render(true);
			if (memcmp(va.data(), pb.data(), va.size() * 4)) { printf("frame %llu: picture differs\n", (unsigned long long)f); bad = true; }
		}
		if (bad) return 1;
		uint8_t ports[input_script::N_PORTS];
		script.apply(f, ports);
		for (model2a *m : { &a, &b }) {
			m->m_in0 = ports[input_script::IN0]; m->m_in1 = ports[input_script::IN1];
			m->m_in2 = ports[input_script::IN2]; m->m_sw = ports[input_script::SW];
			for (const knockout &k : kos)
				if (f >= k.from && f <= k.to && m->workram()[0x2b] == 9 && m->workram()[0x31] == 9) {
					const u32 block = k.player == 1 ? 0x510980 : 0x512980;
					if (int16_t(m->peek32(block + 0x1ac)) > 0 && !(m->peek32(block) & 0x20)) {
						m->write_word(block + 0x1ac, 0);
						m->write_dword(block, m->peek32(block) | 0x20);
					}
				}
		}
		if (every > 0 && (f + 1) % every == 0) {
			const uint64_t it = b.recompiled()->st.interpreted;
			printf("frame %llu ok (mode %02x; interpreted %llu instructions since the last report)\n", (unsigned long long)f + 1, a.game_mode(), (unsigned long long)(it - last_interp));
			last_interp = it;
			fflush(stdout);
		}
	}
	const i960rec::runtime &r = *b.recompiled();
	printf("%llu frames identical (interpreter %.2f s, recompiled %.2f s; %llu instructions interpreted)\n", (unsigned long long)frames, ta, tb, (unsigned long long)r.st.interpreted);
	printf("the i960's time a frame: interpreter %.3f ms, recompiled %.3f ms (%.0f slices a frame)\n", a.cpu().run_ns / 1e6 / double(frames), b.cpu().run_ns / 1e6 / double(frames), double(b.cpu().runs) / double(frames));
	if (!r.interp_pc.empty()) {
		std::vector<std::pair<u32, u32>> top;
		for (u32 i = 0; i < r.interp_pc.size(); i++) if (r.interp_pc[i]) top.push_back({ r.interp_pc[i], i * 4 });
		std::sort(top.rbegin(), top.rend());
		for (size_t i = 0; i < top.size() && i < 10; i++) printf("  interpreted %06x x%u\n", top[i].second, top[i].first);
	}
	return 0;
}
