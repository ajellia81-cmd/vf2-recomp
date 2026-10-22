// The 2D on the GPU: the board hands over its picture in parts (model2a::render_parts), the 2D at
// the arcade's resolution and the 3D at the render scale, and SDL's renderer puts them together in
// a target texture, each 2D pixel an s x s block (nearest sampling), in the board's order: the
// backdrop (moved by smooth motion), the layers over it, the 3D, the layers over the 3D, a 4:3
// screen's sides. The same pixels as model2a::render_into puts together on the CPU: the samples
// fall in the middle of a block's texels, and a pixel of the 3D or the 2D's layers is opaque or
// transparent (alpha 255 or 0), so blending only picks one. The one difference: the background
// filter's smooth enlargement is the GPU's linear sampling, within a level or two of the CPU's
// 8-bit steps (measured by check()).
#pragma once
#include "../machine/model2a.h"
#include "../machine/parallel.h"
#include "d3d11_map.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>

// Rows of pixels (stride pixels apart) sent into a streaming texture's rect (null: all of it):
// locked, copied in on every core, unlocked. SDL_UpdateTexture copies on one thread, and a
// picture is 12 MB at 4x. On Direct3D 11 the texture itself is mapped (d3d11_map.h: SDL's lock
// would make a new staging texture), and the rest of it is undefined after: only the rect is used.
inline bool send_rows(SDL_Texture *t, const SDL_Rect *r, const u32 *src, size_t stride)
{
	void *p;
	int pitch;
	const bool mapped = d3d11_map(t, &p, &pitch);
	if (mapped && r) p = static_cast<uint8_t *>(p) + size_t(r->y) * pitch + size_t(r->x) * 4;
	if (!mapped && !SDL_LockTexture(t, r, &p, &pitch)) return SDL_UpdateTexture(t, r, src, int(stride * 4));
	const int w = r ? r->w : t->w, h = r ? r->h : t->h;
	parallel_rows(h, [&](int a, int b) {
		for (int y = a; y < b; y++) memcpy(static_cast<uint8_t *>(p) + size_t(y) * pitch, src + size_t(y) * stride, size_t(w) * 4);
	});
	if (mapped) d3d11_unmap(t);
	else SDL_UnlockTexture(t);
	return true;
}

class gpu_picture
{
public:
	using parts_t = model2a::picture_parts;
	static constexpr int P = parts_t::PAD, HEIGHT = model2a::HEIGHT, WIDTH = model2a::WIDTH;

	explicit gpu_picture(SDL_Renderer *ren) : m_ren(ren) {}
	~gpu_picture() { free(); }
	void set_output_size(int w, int h) {
		if (w == m_output_w && h == m_output_h) return;
		free();
		m_output_w = std::max(0, w); m_output_h = std::max(0, h);
		m_bad_w = m_bad_s = -1;
	}
	SDL_Texture *target() const { return m_target; }   // the picture put together (LINEAR, for showing it scaled)

	// The textures for a picture w arcade pixels wide at scale s (false: they can't be made here)
	bool ready(int w, int s)
	{
		if (w == m_w && s == m_s) return true;
		if (w == m_bad_w && s == m_bad_s) return false;   // (not that size here: the CPU's way, without trying again each picture)
		free();
		const int W = w * s, H = HEIGHT * s;
		int max = int(SDL_GetNumberProperty(SDL_GetRendererProperties(m_ren), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0));
		if (const char *t = getenv("VF2_GPU_MAX_TEXTURE")) max = atoi(t);   // (tests: a smaller card's limit)
		if (max > 0 && (W > max || H > max || w + 2 * P > max)) {
			fprintf(stderr, "the 2D on the GPU: %d x %d is over this renderer's %d: on the CPU at this size\n", W, H, max);
			m_bad_w = w; m_bad_s = s;
			return false;
		}
		const int out_w = m_output_w ? m_output_w : W, out_h = m_output_h ? m_output_h : H;
		if (max > 0 && (out_w > max || out_h > max)) return false;
		m_target = texture(out_w, out_h, SDL_TEXTUREACCESS_TARGET, SDL_BLENDMODE_NONE, SDL_SCALEMODE_LINEAR);
		m_bd = texture(w + 2 * P, HEIGHT + 2 * P, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_NONE, SDL_SCALEMODE_NEAREST);
		m_mid = texture(w, HEIGHT, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_BLEND, SDL_SCALEMODE_NEAREST);
		m_top = texture(w, HEIGHT, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_BLEND, SDL_SCALEMODE_NEAREST);
		m_3d = texture(W, H, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_BLEND, SDL_SCALEMODE_NEAREST);
		if (!m_target || !m_bd || !m_mid || !m_top || !m_3d) {
			fprintf(stderr, "the 2D on the GPU: no %d x %d textures here (%s): on the CPU at this size\n", W, H, SDL_GetError());
			free();
			m_bad_w = w; m_bad_s = s;
			return false;
		}
		m_w = w; m_s = s;
		return true;
	}

	// The 3D's texture locked for the board to copy its 3D into (render_parts' px3d), until put()
	bool lock(int w, int s, u32 *&px, size_t &stride)
	{
		px = nullptr; stride = 0;
		if (!ready(w, s)) return false;
		void *p;
		int pitch;
		if (!SDL_LockTexture(m_3d, nullptr, &p, &pitch)) return false;
		px = m_lock_px = static_cast<u32 *>(p);
		stride = m_lock_stride = size_t(pitch) / 4;
		m_locked = true;
		return true;
	}
	void unlock() { if (m_locked) { SDL_UnlockTexture(m_3d); m_locked = false; } }
	// The renderer's textures gone (a device reset): the handles let go of, nothing unlocked into them
	void lost() { m_locked = false; free(); }

	// The 2D's textures filled (when it's changed since): put() does it, but a picture whose 3D is
	// drawn alongside the next frame has its 2D sent at once, while it's the board's
	bool upload_2d(const parts_t &p, int ss = 1)
	{
		if (!ready(p.w, p.s / ss)) return false;
		if (p.version != m_version) {
			SDL_UpdateTexture(m_bd, nullptr, p.backdrop, (p.w + 2 * P) * 4);
			SDL_UpdateTexture(m_mid, nullptr, p.mid, p.w * 4);
			SDL_UpdateTexture(m_top, nullptr, p.top, p.w * 4);
			m_version = p.version;
			if (check_every) {   // (the 2D as sent: the board's may be drawn again before the 3D's done)
				m_ck_bd.assign(p.backdrop, p.backdrop + size_t(p.w + 2 * P) * (HEIGHT + 2 * P));
				m_ck_mid.assign(p.mid, p.mid + size_t(p.w) * HEIGHT);
				m_ck_top.assign(p.top, p.top + size_t(p.w) * HEIGHT);
			}
		}
		if (int(p.filter) != m_bd_filter) {
			SDL_SetTextureScaleMode(m_bd, p.filter ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
			m_bd_filter = p.filter;
		}
		return true;
	}

	// The parts put together in target() (the 2D's textures filled when it's changed; the 3D's from
	// where the board drew it, unless it was copied into the locked texture). The render target is
	// the window again after it.
	// ext3d: the 3D drawn on the GPU (gpu3d), the picture's size, premultiplied, in place of the
	// board's. ss: antialiasing 2x or 3x (ext3d averaged from ss times the size): the parts are at
	// that render scale, put together at the picture's (p0.s / ss); a backdrop moved by a part of a
	// picture pixel is drawn at each of the ss x ss places the average takes it from, and averaged
	// (as the CPU averages the picture drawn larger: the same, within a level).
	bool put(const parts_t &p0, SDL_Texture *ext3d = nullptr, int ss = 1)
	{
		parts_t p = p0;
		const int bdx = p0.bdx, bdy = p0.bdy;
		if (ss > 1) p.s = p0.s / ss;
		const bool copied = m_locked && !ext3d;
		if (copied && check_every && (pictures + 1) % uint64_t(check_every) == 0 && p.has3d) {   // (the 3D as it went into the texture, for check())
			const size_t W = size_t(p.w) * p.s;
			m_check3d.assign(W * HEIGHT * p.s, 0);
			for (int y = p.rows3d[0]; y < p.rows3d[1]; y++) memcpy(&m_check3d[y * W], m_lock_px + y * m_lock_stride, W * 4);
			p.map3d = m_check3d.data();
			p.map_stride = W;
		}
		unlock();
		if (!upload_2d(p0, ss)) return false;
		const int s = p.s, w = p.w, W = w * s, H = HEIGHT * s;
		// the 3D: only its rows that can have something
		const int r0 = p.rows3d[0], r1 = p.rows3d[1];
		const bool has3d = p.has3d && r1 > r0;
		const SDL_Rect rows{ 0, r0, W, r1 - r0 };
		const SDL_FRect frows{ 0, float(r0), float(W), float(r1 - r0) };
		if (has3d && !copied && p.map3d) send_rows(m_3d, &rows, p.map3d + size_t(r0) * p.map_stride, p.map_stride);
		float old_x = 1, old_y = 1;
		SDL_GetRenderScale(m_ren, &old_x, &old_y);
		if (!SDL_SetRenderTarget(m_ren, m_target)) return false;
		SDL_SetRenderLogicalPresentation(m_ren, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
		SDL_SetRenderViewport(m_ren, nullptr);
		SDL_SetRenderClipRect(m_ren, nullptr);
		SDL_SetRenderDrawColor(m_ren, 0, 0, 0, 255);
		SDL_RenderClear(m_ren);
		// Keep legacy 2D coordinates, but rasterize the composition directly into
		// its native-sized target. An external native 3D texture maps 1:1 to it.
		SDL_SetRenderScale(m_ren, m_output_w ? float(m_output_w) / W : 1.0f,
			m_output_h ? float(m_output_h) / H : 1.0f);
		const SDL_FRect whole{0, 0, float(W), float(H)};
		if (ss == 1) {
			const SDL_FRect bd_at{ float(-p.bdx - P * s), float(-p.bdy - P * s), float((w + 2 * P) * s), float((HEIGHT + 2 * P) * s) };
			SDL_RenderTexture(m_ren, m_bd, nullptr, &bd_at);
		} else {
			// each sub-pixel's place (the middle of a render-scale pixel, sampled at the picture
			// pixel's middle), the draws averaged as they go (the n-th at 1/n)
			const int nx = bdx % ss || p.filter ? ss : 1, ny = bdy % ss || p.filter ? ss : 1;
			int n = 0;
			for (int j = 0; j < ny; j++)
				for (int i = 0; i < nx; i++) {
					const float ox = nx == 1 ? float(bdx / ss) : (bdx + i + 0.5f) / ss - 0.5f, oy = ny == 1 ? float(bdy / ss) : (bdy + j + 0.5f) / ss - 0.5f;
					const SDL_FRect at{ -ox - P * s, -oy - P * s, float((w + 2 * P) * s), float((HEIGHT + 2 * P) * s) };
					n++;
					SDL_SetTextureBlendMode(m_bd, n == 1 ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
					SDL_SetTextureAlphaModFloat(m_bd, 1.0f / float(n));
					SDL_RenderTexture(m_ren, m_bd, nullptr, &at);
				}
			SDL_SetTextureBlendMode(m_bd, SDL_BLENDMODE_NONE);
			SDL_SetTextureAlphaModFloat(m_bd, 1.0f);
		}
		SDL_RenderTexture(m_ren, m_mid, nullptr, &whole);
		if (ext3d) { if (p.has3d) SDL_RenderTexture(m_ren, ext3d, nullptr, &whole); }
		else if (has3d) SDL_RenderTexture(m_ren, m_3d, &frows, &frows);
		SDL_RenderTexture(m_ren, m_top, nullptr, &whole);
		if (p.sides) {
			const int sw = (w - WIDTH) / 2 * s;
			const SDL_FRect left{ 0, 0, float(sw), float(H) }, right{ float(W - sw), 0, float(sw), float(H) };
			if (p.sides == 1)
				for (int side = 0; side < 2; side++) {
					const u32 c = p.side_colour[side];
					SDL_SetRenderDrawColor(m_ren, (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, 255);
					SDL_RenderFillRect(m_ren, side ? &right : &left);
				}
			else {
				float tw = 0, th = 0;
				if (m_sides) SDL_GetTextureSize(m_sides, &tw, &th);
				if (!m_sides || int(tw) != 2 * sw || int(th) != H) {
					if (m_sides) SDL_DestroyTexture(m_sides);
					m_sides = texture(2 * sw, H, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_NONE, SDL_SCALEMODE_NEAREST);
				}
				if (m_sides) {
					SDL_UpdateTexture(m_sides, nullptr, p.side_px, 2 * sw * 4);
					const SDL_FRect src_l{ 0, 0, float(sw), float(H) }, src_r{ float(sw), 0, float(sw), float(H) };
					SDL_RenderTexture(m_ren, m_sides, &src_l, &left);
					SDL_RenderTexture(m_ren, m_sides, &src_r, &right);
				}
			}
		}
		pictures++;
		if (check_every && pictures % uint64_t(check_every) == 0 && !ext3d && !m_output_w && !m_output_h) check(p);
		const bool restored = SDL_SetRenderTarget(m_ren, nullptr);
		SDL_SetRenderScale(m_ren, old_x, old_y);
		return restored;
	}

	void free()
	{
		unlock();
		for (SDL_Texture **t : { &m_target, &m_bd, &m_mid, &m_top, &m_3d, &m_sides })
			if (*t) { SDL_DestroyTexture(*t); *t = nullptr; }
		m_w = m_s = 0;
		m_bd_filter = -1;
		m_version = ~0ull;
	}

	// Checking (VF2_GPU_CHECK): every check_every-th picture read back and compared with the parts
	// put together on the CPU, as model2a::compose does
	int check_every = 0;
	uint64_t pictures = 0;
	int checked = 0, check_bad = 0, kinds[4] = {};   // (checked: plain, the backdrop moved, filtered, with sides)
	int filter_vs_linear = 0, filter_vs_cpu = 0;    // the filtered backdrop's largest difference from exact linear sampling, from the CPU's
	void report() const
	{
		fprintf(stderr, "gpu check: %d of %llu pictures put together on the GPU checked (%d plain, %d with the backdrop moved, %d filtered, %d with sides), %d different; "
				"the filtered backdrop within %d levels of exact linear sampling, %d of the CPU's\n",
				checked, (unsigned long long)pictures, kinds[0], kinds[1], kinds[2], kinds[3], check_bad, filter_vs_linear, filter_vs_cpu);
	}

private:
	SDL_Renderer *m_ren;
	SDL_Texture *m_target = nullptr, *m_bd = nullptr, *m_mid = nullptr, *m_top = nullptr, *m_3d = nullptr, *m_sides = nullptr;
	int m_w = 0, m_s = 0, m_bd_filter = -1;
	int m_output_w = 0, m_output_h = 0;
	int m_bad_w = -1, m_bad_s = -1;   // a size its textures couldn't be made at
	uint64_t m_version = ~0ull;
	bool m_locked = false;
	u32 *m_lock_px = nullptr;
	size_t m_lock_stride = 0;
	std::vector<u32> m_check3d, m_ck_bd, m_ck_mid, m_ck_top;

	SDL_Texture *texture(int w, int h, SDL_TextureAccess access, SDL_BlendMode blend, SDL_ScaleMode scale_mode)
	{
		SDL_Texture *t = SDL_CreateTexture(m_ren, SDL_PIXELFORMAT_ARGB8888, access, w, h);
		if (t) { SDL_SetTextureBlendMode(t, blend); SDL_SetTextureScaleMode(t, scale_mode); }
		return t;
	}

	void check(const parts_t &p0)
	{
		parts_t p = p0;
		p.backdrop = m_ck_bd.data(); p.mid = m_ck_mid.data(); p.top = m_ck_top.data();
		const int s = p.s, w = p.w, W = w * s, H = HEIGHT * s, pw = w + 2 * P, e = (w - WIDTH) / 2;
		SDL_Surface *got = SDL_RenderReadPixels(m_ren, nullptr);
		SDL_Surface *x = got ? SDL_ConvertSurface(got, SDL_PIXELFORMAT_ARGB8888) : nullptr;
		if (got) SDL_DestroySurface(got);
		if (!x || x->w != W || x->h != H) { fprintf(stderr, "gpu check: no read-back (%s)\n", SDL_GetError()); if (x) SDL_DestroySurface(x); return; }
		auto bd = [&](int a, int b) { return p.backdrop[size_t(std::clamp(b, 0, HEIGHT + 2 * P - 1)) * pw + std::clamp(a, 0, pw - 1)]; };
		auto level = [](u32 c, int ch) { return int((c >> (16 - 8 * ch)) & 0xff); };
		size_t diff = 0, first = 0;
		u32 fa = 0, fb = 0;
		for (int Y = 0; Y < H; Y++) {
			const u32 *row = reinterpret_cast<const u32 *>(static_cast<const uint8_t *>(x->pixels) + size_t(Y) * x->pitch);
			for (int X = 0; X < W; X++) {
				const int xx = X / s, y = Y / s;
				const u32 mid = p.mid[size_t(y) * w + xx], top = p.top[size_t(y) * w + xx];
				const u32 *t3 = p.has3d && Y >= p.rows3d[0] && Y < p.rows3d[1] ? p.map3d + size_t(Y) * p.map_stride + X : nullptr;
				const bool side = p.sides && (X < e * s || X >= (e + WIDTH) * s);
				u32 c = bd((X + p.bdx + P * s) / s, (Y + p.bdy + P * s) / s);
				if (mid) c = mid;
				if (t3 && (*t3 >> 24)) c = *t3;
				if (top) c = top;
				if (side) {
					const bool right = X >= (e + WIDTH) * s;
					c = p.sides == 1 ? p.side_colour[right] : p.side_px[size_t(Y) * 2 * e * s + (right ? X - WIDTH * s : X)];
				}
				const u32 g = row[X] | 0xff000000;
				if (p.filter && !mid && !(t3 && (*t3 >> 24)) && !top && !side) {   // the filtered backdrop
					const double u = (X + 0.5 + p.bdx + P * s) / s - 0.5, v = (Y + 0.5 + p.bdy + P * s) / s - 0.5;
					const int ua = int(std::floor(u)), va = int(std::floor(v));
					const double fu = u - ua, fv = v - va;
					// the CPU's (model2a::compose: 8-bit steps across, then down)
					auto step = [&](int at, int n) { const int num = 2 * at + 1 - s; const int a = num < 0 ? 0 : std::min(num / (2 * s), n - 1); return std::pair(a, num < 0 ? 0 : (num - a * 2 * s) * 256 / (2 * s)); };
					const auto [xa, fx] = step(std::clamp(X + p.bdx, 0, W - 1), w);
					const auto [ya, fy] = step(std::clamp(Y + p.bdy, 0, H - 1), HEIGHT);
					const int xb = std::min(xa + 1, w - 1), yb = std::min(ya + 1, HEIGHT - 1);
					for (int ch = 0; ch < 3; ch++) {
						const double lin = (level(bd(ua, va), ch) * (1 - fu) + level(bd(ua + 1, va), ch) * fu) * (1 - fv) + (level(bd(ua, va + 1), ch) * (1 - fu) + level(bd(ua + 1, va + 1), ch) * fu) * fv;
						filter_vs_linear = std::max(filter_vs_linear, std::abs(int(std::lround(lin)) - level(g, ch)));
						auto q = [&](int a, int b) { return level(bd(a + P, b + P), ch); };
						const int ra = (q(xa, ya) * (256 - fx) + q(xb, ya) * fx) >> 8, rb = (q(xa, yb) * (256 - fx) + q(xb, yb) * fx) >> 8;
						filter_vs_cpu = std::max(filter_vs_cpu, std::abs(((ra * (256 - fy) + rb * fy) >> 8) - level(g, ch)));
					}
					continue;
				}
				if (g != (c | 0xff000000)) { if (!diff++) first = size_t(Y) * W + X, fa = c, fb = g; }
			}
		}
		checked++;
		kinds[0] += !p.bdx && !p.bdy && !p.filter && !p.sides;
		kinds[1] += p.bdx || p.bdy;
		kinds[2] += p.filter;
		kinds[3] += p.sides != 0;
		if (diff && check_bad++ < 10)
			fprintf(stderr, "gpu check: picture %llu: %zu pixels differ (first at %zu,%zu: %08x on the CPU, %08x on the GPU; scale %d, backdrop moved %d,%d, sides %d)\n",
					(unsigned long long)pictures, diff, first % W, first / W, fa, fb, s, p.bdx, p.bdy, p.sides);
		SDL_DestroySurface(x);
	}
};
