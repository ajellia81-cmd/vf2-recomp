// The GPU renderer (src/gpu/gpu3d.h), checked: the 3D shown drawn both ways, the CPU's renderer
// and the GPU (Metal on a Mac, else SDL_GPU: Vulkan or Direct3D 12), each pixel's polygon compared (model2_video::owner_map_shown) and its
// colour (the CPU's 3D picture, before the 2D: draw_layer_into).
//   gpu3dcmp roms/vf2.zip FRAMES SCRIPT|- SCALES ASPECT EVERY [OUTDIR [PNG_FRAMES]]
//   e.g. gpu3dcmp roms/vf2.zip 6000 tools/scripts/start_game.txt 1,3 1.3333 250 build/gpu3d 2000,5000
// A line a frame compared (and scale): the pixels either covers, those with the same polygon,
// and the others: on an edge (beside a pixel the CPU gives another polygon) or inside, covered by
// the CPU alone, the GPU alone, or both with another polygon. Then the colours, of the pixels
// either covers: the same, within 8 in each channel, the largest difference, and where the
// different ones are (another polygon; the same one, solid or textured). PNG_FRAMES: the frames
// whose pictures (CPU, GPU, a heatmap of the differences, the coverage's, and the picture
// itself) go in OUTDIR. Its output is
// ROM-derived: keep it out of git (build/).
#include "../src/machine/model2a.h"
#include "../src/host/input_script.h"
#include "../src/host/png.h"
#include "../src/gpu/gpu3d.h"
#ifdef GPU3DCMP_SDLGPU
#include <SDL3/SDL.h>
#endif
#include <chrono>
#include <cstring>
#include <filesystem>
#include <set>
#include <sstream>

static std::vector<int> ints(const char *s)
{
	std::vector<int> v; std::stringstream ss(s); std::string t;
	while (std::getline(ss, t, ',')) if (!t.empty()) v.push_back(atoi(t.c_str()));
	return v;
}

int main(int argc, char **argv)
{
	if (argc < 7) { fprintf(stderr, "usage: gpu3dcmp <vf2.zip> <frames> <script|-> <scales> <aspect> <every> [outdir [png frames]]\n"); return 2; }
	romset::result roms = romset::load(argv[1]);
	if (!roms.ok) { fprintf(stderr, "%s\n", roms.error.c_str()); return 1; }
	const int frames = atoi(argv[2]);
	const std::vector<int> scales = ints(argv[4]);
	const int every = std::max(1, atoi(argv[6]));
	const std::string out = argc > 7 ? argv[7] : "build/gpu3d";
	std::set<int> pngs; if (argc > 8) for (int f : ints(argv[8])) pngs.insert(f);
	std::filesystem::create_directories(out);
#ifdef GPU3DCMP_SDLGPU
	// SDL_GPU (Vulkan, Direct3D 12): headless, SDL_VIDEODRIVER=offscreen without a display
	if (!SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
	std::unique_ptr<gpu3d> gpu = gpu3d_create_sdlgpu();
#else
	std::unique_ptr<gpu3d> gpu = gpu3d_create_metal();
#endif
	if (!gpu) { fprintf(stderr, "no GPU renderer\n"); return 1; }
	if (getenv("GPU3D_REPEAT")) gpu->test_repeat = atoi(getenv("GPU3D_REPEAT"));
	if (getenv("GPU3D_FAST")) gpu->set_quality(gpu3d_quality::fast);   // (GPU3D_SPANS=0/1/2: the triangles, in place of the exact way)
	model2a m(std::move(roms));
	m.set_sound(false); m.reset(); m.defer_3d(true); m.set_aspect(float(atof(argv[5])));
	input_script s; std::string err;
	if (strcmp(argv[3], "-") && !s.load(argv[3], err)) { fprintf(stderr, "%s\n", err.c_str()); return 1; }
	struct total { long long cover = 0, same = 0, edge = 0, inner = 0, ident = 0, near8 = 0, d_owner = 0, d_solid = 0, d_tex = 0; int maxerr = 0; double min_ident = 100, min_near8 = 100, cpu_ms = 0, own_ms = 0, gpu_ms = 0; int n = 0; };
	std::vector<total> tot(scales.size());
	std::vector<gpu3d_poly> list;
	std::vector<u32> co, go, cc, gg;
	gpu3d_rams rams;
	using clk = std::chrono::steady_clock;
	auto ms = [](clk::time_point a) { return std::chrono::duration<double, std::milli>(clk::now() - a).count(); };
	for (int f = 0; f < frames; f++) {
		m.run_frame();
		if (f % every == 0 && f > 0) {
			for (size_t si = 0; si < scales.size(); si++) {
				m.set_render_scale(scales[si]);
				auto t0 = clk::now();
				m.render(true);   // (the deferred 3D drawn: the CPU's picture, with the 2D)
				model2_video &v = m.video();
				v.export_shown(list);
				if (list.empty()) continue;
				int w, h; v.render_size(w, h);
				const size_t N = size_t(w) * h;
				co.assign(N, 0); go.assign(N, 0); cc.assign(N, 0); gg.assign(N, 0);
				t0 = clk::now();
				v.draw_layer_into(v.shown_picture(0), cc.data(), w, 0, h);   // the CPU's 3D again (all cores), timed
				const double cpu_ms = ms(t0);
				t0 = clk::now();
				v.owner_map_shown(co.data(), w);
				const double own_ms = ms(t0);
				double gpu_ms = 0;
				t0 = clk::now();
				v.gpu_rams(rams);
				gpu->set_rams(rams);
				if (!gpu->draw(list.data(), list.size(), w, h, scales[si], go.data(), w, gg.data(), w, &gpu_ms)) { fprintf(stderr, "frame %d: GPU draw failed\n", f); continue; }
				const double gpu_wall = ms(t0);
				long long cover = 0, same = 0, c_only[2] = {}, g_only[2] = {}, other[2] = {};
				long long ident = 0, near8 = 0, d_owner = 0, d_solid = 0, d_tex = 0;
				int maxerr = 0;
				std::vector<u32> diff(pngs.count(f) ? N : 0), heat(pngs.count(f) ? N : 0);
				for (int y = 0; y < h; y++)
					for (int x = 0; x < w; x++) {
						const size_t i = size_t(y) * w + x;
						const u32 a = co[i], b = go[i];
						if (a == 0xffffffffu && b == 0xffffffffu && !cc[i] && !gg[i]) continue;
						cover++;
						// the colours
						const u32 ca = cc[i], cb = gg[i];
						int e = 0;
						if ((ca == 0) != (cb == 0)) e = 255;
						else for (int sh = 0; sh < 24; sh += 8) e = std::max(e, std::abs(int((ca >> sh) & 255) - int((cb >> sh) & 255)));
						if (ca == cb) ident++;
						if (e <= 8) near8++;
						maxerr = std::max(maxerr, e);
						if (ca != cb) {
							if (a != b) d_owner++;
							else if (a != 0xffffffffu && list[a].renderer & 2) d_tex++;
							else d_solid++;
						}
						if (!heat.empty()) heat[i] = ca == cb ? (ca ? 0x202020 : 0) : e == 255 ? 0xff00ff : e > 8 ? 0xff0000 | (std::min(255, e * 4) << 8) : 0x0040ff | (e * 24 << 16);
						// the coverage
						if (a == 0xffffffffu && b == 0xffffffffu) continue;
						if (a == b) { same++; continue; }
						bool edge = false;   // beside a pixel the CPU gives another polygon
						for (int d = 0; d < 4 && !edge; d++) {
							const int xx = x + (d == 0) - (d == 1), yy = y + (d == 2) - (d == 3);
							if (xx >= 0 && yy >= 0 && xx < w && yy < h && co[size_t(yy) * w + xx] != a) edge = true;
						}
						long long *c = a == 0xffffffffu ? g_only : b == 0xffffffffu ? c_only : other;
						c[edge ? 0 : 1]++;
						if (!diff.empty()) diff[i] = a == 0xffffffffu ? 0x00ff00 : b == 0xffffffffu ? 0xff0000 : 0xffff00;
					}
				const int sc = scales[si];
				const double pc = cover ? 100.0 / cover : 0;
				printf("frame %5d %dx: %5zu polygons, %8lld px; owner same %.4f%% (edge %lld/%lld/%lld inside %lld/%lld/%lld); colour same %.3f%% within8 %.3f%% max %d (differ: other polygon %lld, solid %lld, textured %lld); ms cpu 3D %.2f owner %.2f gpu %.2f (wall %.2f)\n",
					f, sc, list.size(), cover, (cover - (c_only[0] + g_only[0] + other[0] + c_only[1] + g_only[1] + other[1])) * pc,
					c_only[0], g_only[0], other[0], c_only[1], g_only[1], other[1], ident * pc, near8 * pc, maxerr, d_owner, d_solid, d_tex, cpu_ms, own_ms, gpu_ms, gpu_wall);
				total &t = tot[si];
				t.cover += cover; t.same += cover - (c_only[0] + g_only[0] + other[0] + c_only[1] + g_only[1] + other[1]);
				t.edge += c_only[0] + g_only[0] + other[0]; t.inner += c_only[1] + g_only[1] + other[1];
				t.ident += ident; t.near8 += near8; t.maxerr = std::max(t.maxerr, maxerr); t.d_owner += d_owner; t.d_solid += d_solid; t.d_tex += d_tex;
				t.min_ident = std::min(t.min_ident, ident * pc); t.min_near8 = std::min(t.min_near8, near8 * pc);
				t.cpu_ms += cpu_ms; t.own_ms += own_ms; t.gpu_ms += gpu_ms; t.n++;
				if (!diff.empty()) {
					for (size_t i = 0; i < N; i++) { cc[i] &= 0xffffff; gg[i] &= 0xffffff; }
					char base[512]; snprintf(base, sizeof base, "%s/f%05d_%dx", out.c_str(), f, sc);
					write_png((std::string(base) + "_cpu.png").c_str(), cc.data(), w, h);
					write_png((std::string(base) + "_gpu.png").c_str(), gg.data(), w, h);
					write_png((std::string(base) + "_heat.png").c_str(), heat.data(), w, h);
					write_png((std::string(base) + "_cover.png").c_str(), diff.data(), w, h);
					const bitmap_rgb32 &shot = m.render(true);   // the picture itself (CPU), to see the scene
					write_png((std::string(base) + "_shot.png").c_str(), shot.data(), shot.width(), shot.height());
				}
			}
			if (scales.size() != 1 || scales[0] != 1) m.set_render_scale(1);
		}
		uint8_t p[4]; s.apply(f, p); m.m_in0 = p[0]; m.m_in1 = p[1]; m.m_in2 = p[2]; m.m_sw = p[3];
	}
	for (size_t si = 0; si < scales.size(); si++) {
		const total &t = tot[si];
		printf("all %dx: %d frames, %lld px; owner same %.4f%%, differ on edges %lld, inside %lld; colour same %.3f%% (worst frame %.3f%%), within8 %.3f%% (worst %.3f%%), max %d; differ: other polygon %lld, solid %lld, textured %lld; mean ms cpu 3D %.2f, owner %.2f, gpu %.2f\n",
			scales[si], t.n, t.cover, t.cover ? 100.0 * t.same / t.cover : 100.0, t.edge, t.inner,
			t.cover ? 100.0 * t.ident / t.cover : 100.0, t.min_ident, t.cover ? 100.0 * t.near8 / t.cover : 100.0, t.min_near8, t.maxerr,
			t.d_owner, t.d_solid, t.d_tex, t.n ? t.cpu_ms / t.n : 0, t.n ? t.own_ms / t.n : 0, t.n ? t.gpu_ms / t.n : 0);
	}
}
