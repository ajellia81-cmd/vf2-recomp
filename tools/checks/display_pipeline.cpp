// ROM-free SDL/Metal composition, filter, resize/recreation and brightness checks.
#include "../../src/host/gpu_picture.h"
#include "../../src/host/display_color.h"
#include <cstdio>
#include <vector>
#include <cstdlib>

static void require(bool ok, const char *what) {
	if (!ok) { std::fprintf(stderr, "FAIL: %s (%s)\n", what, SDL_GetError()); std::exit(1); }
}
static uint32_t pixel(SDL_Renderer *ren, SDL_Texture *target, int x, int y) {
	require(SDL_SetRenderTarget(ren, target), "read target");
	SDL_Surface *raw = SDL_RenderReadPixels(ren, nullptr);
	require(raw != nullptr, "read pixels");
	SDL_Surface *s = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ARGB8888);
	SDL_DestroySurface(raw);
	require(s != nullptr, "convert readback");
	uint32_t c = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(s->pixels) + y * s->pitch)[x];
	SDL_DestroySurface(s);
	require(SDL_SetRenderTarget(ren, nullptr), "restore target");
	return c | 0xff000000;
}
int main() {
	require(SDL_Init(SDL_INIT_VIDEO), "video init");
	SDL_Window *win = SDL_CreateWindow("Synthetic display checks", 320, 240, SDL_WINDOW_HIDDEN);
	require(win != nullptr, "window");
	SDL_Renderer *ren = SDL_CreateRenderer(win, "metal");
	require(ren != nullptr, "Metal renderer");
	constexpr int w = 808, h = 384, pad = model2a::picture_parts::PAD;
	std::vector<u32> backdrop(size_t(w + 2*pad) * (h + 2*pad), 0xff204060);
	std::vector<u32> clear(size_t(w) * h, 0);
	for (int launch = 0; launch < 2; ++launch) {
		gpu_picture picture(ren);
		for (int scale : {0, 1, 2, 3, 4, 5, 6, 0}) {
			const int s = scale ? scale : 1;
			const int W = scale ? w * s : 2868, H = scale ? h * s : 1320;
			picture.set_output_size(scale ? 0 : W, scale ? 0 : H);
			model2a::picture_parts p{};
			p.w = w; p.s = s; p.version = 1;
			p.backdrop = backdrop.data(); p.mid = p.top = clear.data();
			p.has3d = p.gpu3d = true; p.rows3d[1] = h * s;
			std::vector<u32> fg(size_t(W) * H, 0);
			for (int y = 0; y < H; ++y) for (int x = W/2; x < W; ++x) fg[size_t(y)*W+x] = 0xff6080a0;
			SDL_Texture *ext = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
			require(ext != nullptr, "external 3D texture");
			require(SDL_UpdateTexture(ext, nullptr, fg.data(), W * 4), "upload external 3D");
			require(SDL_SetTextureBlendMode(ext, SDL_BLENDMODE_BLEND_PREMULTIPLIED), "premultiplied blend");
			for (bool filter : {false, true, false}) {
				p.filter = filter;
				require(picture.put(p, ext), "compose after filter/resize");
				require(int(picture.target()->w) == W && int(picture.target()->h) == H, "selected composition dimensions");
				require(pixel(ren, picture.target(), W/4, H/2) == 0xff204060, "original backdrop RGB unchanged");
				require(pixel(ren, picture.target(), W*3/4, H/2) == 0xff6080a0, "original 3D RGB unchanged");
			}
			SDL_Texture *output = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 64, 64);
			require(output != nullptr, "brightness output");
			for (int brightness : {100, 150, 200, 100}) {
				require(SDL_SetRenderTarget(ren, output), "brightness target");
				require(draw_game_picture(ren, picture.target(), brightness), "brightness draw");
				const uint32_t expected = brightness == 100 ? 0xff204060 : brightness == 150 ? 0xff306090 : 0xff4080c0;
				require(pixel(ren, output, 16, 32) == expected, "brightness RGB including boost above 100%");
			}
			SDL_DestroyTexture(output); SDL_DestroyTexture(ext);
		}
	}
	SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
	std::puts("PASS: Metal composition RGB, background filter toggles, 1x-6x/native targets, recreation and 100-200% brightness");
}
