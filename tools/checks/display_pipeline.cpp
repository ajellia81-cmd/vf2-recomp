// ROM-free SDL/Metal composition, filter, resize/recreation and brightness checks.
#include "../../src/host/gpu_picture.h"
#include "../../src/host/display_color.h"
#include "../../src/host/screen_filter.h"
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
			// Simulate leaked composition/UI coordinates before drawing to the window.
			const SDL_Rect stale{0, 0, 8, 8};
			SDL_SetRenderScale(ren, 4, 4);
			SDL_SetRenderViewport(ren, &stale);
			SDL_SetRenderClipRect(ren, &stale);
			require(prepare_game_view(ren, 2868, 1320), "reset stale window state");
			require(draw_game_picture(ren, picture.target(), 100), "full game view");
			SDL_FRect area{};
			require(SDL_GetRenderLogicalPresentationRect(ren, &area), "game viewport");
			require(pixel(ren, nullptr, int(area.x + area.w/4), int(area.y + area.h/2)) == 0xff204060, "full picture after stale scale/clip");
			require(prepare_overlay_view(ren), "UI pixel coordinates");
			float sx = 0, sy = 0; SDL_GetRenderScale(ren, &sx, &sy);
			require(sx == 1 && sy == 1 && !SDL_RenderClipEnabled(ren) && !SDL_RenderViewportSet(ren), "UI is not enlarged or clipped");
			SDL_DestroyTexture(output); SDL_DestroyTexture(ext);
		}
	}
	// A native output at board scale 1 must still visibly smooth a checkerboard.
	{
		gpu_picture picture(ren); picture.set_output_size(2868, 1320);
		for (int y = 0; y < h + 2*pad; ++y) for (int x = 0; x < w + 2*pad; ++x)
			backdrop[size_t(y)*(w+2*pad)+x] = x % 2 ? 0xffc0c0c0 : 0xff404040;
		model2a::picture_parts p{}; p.w=w; p.s=1; p.version=100;
		p.backdrop=backdrop.data(); p.mid=p.top=clear.data();
		for (bool smooth : {false, true, false}) {
			p.filter=smooth; require(picture.put(p), "checkerboard background filter");
			SDL_SetRenderTarget(ren, picture.target());
			SDL_Surface *raw=SDL_RenderReadPixels(ren,nullptr);
			require(raw!=nullptr,"checkerboard readback");
			SDL_Surface *rgb=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_ARGB8888); SDL_DestroySurface(raw);
			require(rgb!=nullptr,"checkerboard conversion");
			const u32 *row=reinterpret_cast<u32 *>(static_cast<uint8_t *>(rgb->pixels)+100*rgb->pitch);
			int between=0;
			for(int x=32;x<500;++x) {int c=row[x]&255; between += c>64 && c<192;}
			require(smooth ? between>100 : between==0,"Off/Smooth/Off changes actual native background pixels");
			SDL_DestroySurface(rgb); SDL_SetRenderTarget(ren,nullptr);
		}
	}
	// Screen filters must change pixels, with a visible scanline contrast at native height.
	{
		screen_filter glass(ren);
		SDL_Texture *target=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,96,1320);
		require(target!=nullptr,"screen filter output");
		for(int kind : {0,1,2,0}) {
			SDL_SetRenderTarget(ren,target); prepare_overlay_view(ren);
			SDL_SetRenderDrawColor(ren,128,128,128,255); SDL_RenderClear(ren);
			require(glass.draw(kind),"draw screen filter");
			const u32 a=pixel(ren,target,12,660),b=pixel(ren,target,12,661);
			if(!kind) require(a==0xff808080 && b==a,"Off restores original screen pixels");
			if(kind==1) require(std::abs(int(a&255)-int(b&255))>=20,"Scanlines are visible");
			if(kind==2) require(std::abs(int((b>>16)&255)-int((b>>8)&255))>=10,"CRT RGB stripes are visible");
		}
		glass.clear(); SDL_DestroyTexture(target);
	}
	SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
	std::puts("PASS: Metal composition RGB, background filter toggles, 1x-6x/native targets, recreation, stale window/UI coordinates, visible background/screen filters and 100-200% brightness");
}
