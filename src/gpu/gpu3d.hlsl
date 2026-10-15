// license:BSD-3-Clause
// copyright-holders:R. Belmont, Olivier Galibert, ElSemi, Angelo Salese, Matthew Daniels, Ville Linde, Aaron Giles
// The 3D on the GPU, SDL_GPU's compute (src/gpu/gpu3d_sdlgpu.cpp): Vulkan (SPIR-V) and Direct3D 12
// (DXIL), the exact way of gpu3d_metal.mm's shaders in HLSL. The texel filtering, mipmapping, luma
// and colour are the CPU renderer's (model2_video.cpp: fetch_bilinear_texel, fast_log2 and
// draw_scanline_tex), which are MAME's (sega/model2rd.ipp; fast_log2 from video/voodoo_render.cpp),
// and the scan conversion is poly.h's render_triangle and render_polygon (MAME's
// devices/video/poly.h, Ville Linde and Aaron Giles), BSD-3-Clause, copyright-holders as above,
// written for the GPU. Low's edge smoothing is model2a::edge_smooth's (ours).
//
// The sums are an x86-64 build's (the Windows and Linux CPU renderer's, the picture to match):
// each multiply and add rounded on its own (no fused multiply-add: x86-64 builds don't fuse), so
// every float sum here is `precise` (no contraction by the compiler or the driver); division
// correctly rounded (fdiv: a GPU's divide is within 2.5 ulp, not IEEE), and a float made an int as
// SSE's cvttss2si makes it (out of range or NaN: INT_MIN).
//
// Built once per kernel (-D K_CLEAR, K_SPANS, K_SHADE, K_OUT, K_EDGE) by tools/gpu/shaders.sh into
// src/gpu/gpu3d_shaders.h. The resources follow SDL_GPU's layout for compute: read-only storage
// buffers t0.. in space 0 (SPIR-V set 0), read-write storage textures then buffers u0.. in space 1
// (set 1), uniforms b0 in space 2 (set 2).
//
// Every buffer is a byte-address buffer (ByteAddressBuffer, loads and stores by byte offset), not a
// structured buffer: SDL_GPU's Direct3D 12 backend binds storage buffers as raw views (R32_TYPELESS,
// D3D12_BUFFER_*_FLAG_RAW), and a raw view in a structured buffer's slot is undefined there. NVIDIA's
// recent drivers read it anyway; AMD's (a Radeon 780M) and older NVIDIA (a GTX 970) drew nothing
// (vf2-recomp #3, #8). Vulkan has no such distinction (SPIR-V's storage buffers are the same either way).
#define LD(b, i) (b).Load((i) * 4u)                         // the i-th 32-bit word
#define LDF(b, i) asfloat((b).Load((i) * 4u))
#define ST(b, i, v) (b).Store((i) * 4u, (v))

// ---- the CPU's arithmetic ----
// a / b, correctly rounded (round to nearest even), denormals and the specials as IEEE has them
// (NaN as SSE: the first NaN operand quietened, else the default NaN 0xffc00000)
float fdiv(float a, float b)
{
	const uint ua = asuint(a), ub = asuint(b);
	const uint sign = (ua ^ ub) & 0x80000000u;
	int ea = int((ua >> 23) & 0xffu), eb = int((ub >> 23) & 0xffu);
	uint ma = ua & 0x7fffffu, mb = ub & 0x7fffffu;
	if (ea == 255 || eb == 255) {
		if (ea == 255 && ma != 0u) return asfloat(ua | 0x00400000u);
		if (eb == 255 && mb != 0u) return asfloat(ub | 0x00400000u);
		if (ea == 255 && eb == 255) return asfloat(0xffc00000u);   // inf / inf
		if (ea == 255) return asfloat(sign | 0x7f800000u);         // inf / x
		return asfloat(sign);                                       // x / inf
	}
	if (eb == 0 && mb == 0u) {
		if (ea == 0 && ma == 0u) return asfloat(0xffc00000u);      // 0 / 0
		return asfloat(sign | 0x7f800000u);                         // x / 0
	}
	if (ea == 0 && ma == 0u) return asfloat(sign);                 // 0 / x
	// the significands, 24 bits with the leading one (denormals normalised)
	if (ea == 0) { const int sh = 23 - int(firstbithigh(ma)); ma <<= sh; ea = 1 - sh; } else ma |= 0x800000u;
	if (eb == 0) { const int sh = 23 - int(firstbithigh(mb)); mb <<= sh; eb = 1 - sh; } else mb |= 0x800000u;
	int e = ea - eb + 127;
	if (ma < mb) { ma <<= 1; e--; }
	// 26 bits of quotient (24, then two more for the rounding) and what's left over
	uint q = 0u, r = ma;
	[unroll] for (int i = 0; i < 26; i++) {
		q <<= 1;
		if (r >= mb) { r -= mb; q |= 1u; }
		r <<= 1;
	}
	bool sticky = r != 0u;
	if (e <= 0) {   // a denormal result
		const int sh = 1 - e;
		if (sh >= 26) { sticky = sticky || q != 0u; q = 0u; }
		else { sticky = sticky || (q & ((1u << sh) - 1u)) != 0u; q >>= sh; }
		e = 0;
	}
	uint m = q >> 2;
	const bool g = (q & 2u) != 0u, rest = (q & 1u) != 0u || sticky;
	if (g && (rest || (m & 1u) != 0u)) m++;
	if (e == 0) return asfloat(sign | m);   // (rounded up to the smallest normal: m's carry is the exponent's 1)
	if (m == 0x1000000u) { m >>= 1; e++; }
	if (e >= 255) return asfloat(sign | 0x7f800000u);
	return asfloat(sign | (uint(e) << 23) | (m & 0x7fffffu));
}

// int(x) as x86-64's cvttss2si: toward zero; out of range or NaN, INT_MIN
int ftoi(float x)
{
	return abs(x) < 2147483648.0f ? int(x) : int(0x80000000u);
}

// poly.h's round_coordinate (x86-64)
int round_coordinate(float value)
{
	if (value >= 2147483648.0f) return 0x7fffffff;
	precise const float ipart = floor(value);
	if (ipart < -2147483648.0f) return int(0x80000000u);
	precise const float fpart = value - ipart;
	return ftoi(ipart) + ((fpart > 0.5f) ? 1 : 0);
}

// ---- what the kernels share ----
struct pinfo {
	uint flags;       // 1 checker, 2 textured, 4 translucent, 8 sheet (texture RAM 1 is sheet 0)
	uint colour;      // solid: 0xffRRGGBB
	uint pal, lumabase, luma;
	int texlod;
	uint texw, texh, texx, texy;
	uint wrapx, wrapy, mirrorx, mirrory, utex, utexminlod, utexx, utexy;
	int max_level;
	uint n;           // vertices
	int clipx;        // the window's left column, right column + 1 (render-scale pixels)
	int clipx1;
	int y0;           // its first row, the row records' and the kept parameters' places
	uint rowbase, ckbase, ckstride;
};
struct pverts { float x[8]; float y[8]; float p[24]; };   // the vertices (render-scale), 1/z, u/z, v/z (p[k * 3 + param])
// a row's span (sx: its first pixel drawn, the checker's skip made; stop: past its last) and the
// step from one pixel drawn to the next
struct rowrec { int sx; int stop; float d0; float d1; float d2; };
#define CK 16   // the parameters kept every CK pixels drawn

#if defined(K_SPANS) || defined(K_SHADE)
ByteAddressBuffer polys : register(t0, space0);
pinfo poly_at(uint i) { return polys.Load<pinfo>(i * sizeof(pinfo)); }
#endif
#if defined(K_SPANS)
ByteAddressBuffer verts : register(t1, space0);
ByteAddressBuffer jobpoly : register(t2, space0);
ByteAddressBuffer jobbase : register(t3, space0);
ByteAddressBuffer tex0 : register(t4, space0);
ByteAddressBuffer tex1 : register(t5, space0);
RWByteAddressBuffer rows : register(u0, space1);
RWByteAddressBuffer ck : register(u1, space1);
RWByteAddressBuffer owner : register(u2, space1);
cbuffer spans_k : register(b0, space2) { uint k_jobs; uint k_W; uint k_H; uint k_total; uint4 k_log2[8]; };
#define HAVE_TEX 1
#endif
#if defined(K_SHADE)
ByteAddressBuffer rows : register(t1, space0);
ByteAddressBuffer ck : register(t2, space0);
ByteAddressBuffer owner : register(t3, space0);
ByteAddressBuffer tex0 : register(t4, space0);
ByteAddressBuffer tex1 : register(t5, space0);
// the luma RAM (bytes, four a word), colorxlat (16 bits, two a word) and gamma (bytes), one after
// another (SDL_GPU takes 8 read-only buffers at most)
ByteAddressBuffer luts : register(t6, space0);
#define LUMA_AT 0u
#define XLAT_AT 0x2000u
#define GAMMA_AT 0x5000u
RWByteAddressBuffer outbuf : register(u0, space1);
cbuffer shade_k : register(b0, space2) { uint k_W; uint k_H; uint k_pad0; uint k_pad1; uint4 k_log2[8]; };
#define HAVE_TEX 1
#endif

#ifdef HAVE_TEX
// poly.h's extent for row y: the span [istartx, istopx) after the window's columns [cx0, cx1),
// its 1/z, u/z, v/z at its first pixel and their step across (render_triangle, render_polygon,
// each sum as poly.h has it, rounded a step at a time)
void extent(pinfo o, pverts pv, int y, int cx0, int cx1, out int istartx, out int istopx, out float3 start, out float3 dpdx)
{
	precise const float fully = float(y) + 0.5f;
	precise float startx, stopx;
	const uint n = o.n;
	start = float3(0, 0, 0); dpdx = float3(0, 0, 0);
	if (n == 3) {
		int i1 = 0, i2 = 1, i3 = 2;
		if (pv.y[i2] < pv.y[i1]) { int t = i1; i1 = i2; i2 = t; }
		if (pv.y[i3] < pv.y[i2]) { int t = i2; i2 = i3; i3 = t; if (pv.y[i2] < pv.y[i1]) { t = i1; i1 = i2; i2 = t; } }
		const float x1 = pv.x[i1], y1 = pv.y[i1], x2 = pv.x[i2], y2 = pv.y[i2], x3 = pv.x[i3], y3 = pv.y[i3];
		precise const float dxdy_v1v2 = (y2 == y1) ? 0.0f : fdiv(x2 - x1, y2 - y1);
		precise const float dxdy_v1v3 = (y3 == y1) ? 0.0f : fdiv(x3 - x1, y3 - y1);
		precise const float dxdy_v2v3 = (y3 == y2) ? 0.0f : fdiv(x3 - x2, y3 - y2);
		precise float3 ps = float3(0, 0, 0), pdx = float3(0, 0, 0), pdy = float3(0, 0, 0);
		if (o.flags & 2u) {
			precise const float a00 = y2 - y3, a01 = x3 - x2, a02 = x2 * y3 - x3 * y2;
			precise const float a10 = y3 - y1, a11 = x1 - x3, a12 = x3 * y1 - x1 * y3;
			precise const float a20 = y1 - y2, a21 = x2 - x1, a22 = x1 * y2 - x2 * y1;
			precise const float det = a02 + a12 + a22;
			[unroll] for (int k = 0; k < 3; k++) {
				const float p1 = pv.p[i1 * 3 + k], p2 = pv.p[i2 * 3 + k], p3 = pv.p[i3 * 3 + k];
				if (abs(det) < 0.00001f) { pdx[k] = 0.0f; pdy[k] = 0.0f; ps[k] = p1; }
				else {
					precise const float idet = fdiv(1.0f, det);
					pdx[k] = idet * (p1 * a00 + p2 * a10 + p3 * a20);
					pdy[k] = idet * (p1 * a01 + p2 * a11 + p3 * a21);
					ps[k] = idet * (p1 * a02 + p2 * a12 + p3 * a22);
				}
			}
		}
		startx = x1 + (fully - y1) * dxdy_v1v3;
		stopx = fully < y2 ? x1 + (fully - y1) * dxdy_v1v2 : x2 + (fully - y2) * dxdy_v2v3;
		istartx = round_coordinate(startx); istopx = round_coordinate(stopx);
		if (istartx > istopx) { const int t = istartx; istartx = istopx; istopx = t; }
		istartx = max(istartx, cx0); istopx = min(istopx, cx1);
		precise const float fullstartx = float(istartx) + 0.5f;
		[unroll] for (int k2 = 0; k2 < 3; k2++) { precise float s = ps[k2] + fullstartx * pdx[k2] + fully * pdy[k2]; start[k2] = s; }
		dpdx = pdx;
	} else {
		int minv = 0, maxv = 0;
		for (uint k = 1; k < n; k++) {
			if (pv.y[k] < pv.y[minv]) minv = int(k);
			else if (pv.y[k] > pv.y[maxv]) maxv = int(k);
		}
		// the edges, forward and backward from the top vertex, horizontal ones left out
		int fa[7], fb[7], ba[7], bb[7], nf = 0, nb = 0;
		for (int c = minv; c != maxv; c = (c == int(n) - 1) ? 0 : c + 1) {
			const int d = (c == int(n) - 1) ? 0 : c + 1;
			if (pv.y[c] != pv.y[d]) { fa[nf] = c; fb[nf] = d; nf++; }
		}
		for (int c2 = minv; c2 != maxv; c2 = (c2 == 0) ? int(n) - 1 : c2 - 1) {
			const int d = (c2 == 0) ? int(n) - 1 : c2 - 1;
			if (pv.y[c2] != pv.y[d]) { ba[nb] = c2; bb[nb] = d; nb++; }
		}
		if (nf == 0 || nb == 0) { istartx = istopx = 0; return; }
		precise const float fdx = (pv.x[fb[0]] - pv.x[fa[0]]) * fdiv(1.0f, pv.y[fb[0]] - pv.y[fa[0]]);
		precise const float bdx = (pv.x[bb[0]] - pv.x[ba[0]]) * fdiv(1.0f, pv.y[bb[0]] - pv.y[ba[0]]);
		const bool fleft = (fa[0] == ba[0] && fdx < bdx) || (fa[0] != ba[0] && pv.x[fa[0]] < pv.x[ba[0]]);
		int la, lb, ra, rb;
		{
			int li = 0, ri = 0;
			const int nl = fleft ? nf : nb, nr = fleft ? nb : nf;
			while (li < nl - 1 && fully > pv.y[fleft ? fb[li] : bb[li]] && fully < pv.y[maxv]) li++;
			while (ri < nr - 1 && fully > pv.y[fleft ? bb[ri] : fb[ri]] && fully < pv.y[maxv]) ri++;
			la = fleft ? fa[li] : ba[li]; lb = fleft ? fb[li] : bb[li];
			ra = fleft ? ba[ri] : fa[ri]; rb = fleft ? bb[ri] : fb[ri];
		}
		precise const float looy = fdiv(1.0f, pv.y[lb] - pv.y[la]), rooy = fdiv(1.0f, pv.y[rb] - pv.y[ra]);
		precise const float lx = (pv.x[lb] - pv.x[la]) * looy, rx = (pv.x[rb] - pv.x[ra]) * rooy;
		startx = pv.x[la] + (fully - pv.y[la]) * lx;
		stopx = pv.x[ra] + (fully - pv.y[ra]) * rx;
		istartx = round_coordinate(startx); istopx = round_coordinate(stopx);
		if (istartx > istopx) { const int t = istartx; istartx = istopx; istopx = t; }
		istartx = max(istartx, cx0); istopx = min(istopx, cx1);
		if (o.flags & 2u) {
			precise const float ldy = fully - pv.y[la], rdy = fully - pv.y[ra];
			precise const float oox = fdiv(1.0f, stopx - startx);
			[unroll] for (int k = 0; k < 3; k++) {
				precise const float ldp = (pv.p[lb * 3 + k] - pv.p[la * 3 + k]) * looy;
				precise const float rdp = (pv.p[rb * 3 + k] - pv.p[ra * 3 + k]) * rooy;
				precise const float lparam = pv.p[la * 3 + k] + ldy * ldp;
				precise const float rparam = pv.p[ra * 3 + k] + rdy * rdp;
				precise const float d = (rparam - lparam) * oox;
				precise const float s = lparam + (float(istartx) + 0.5f - startx) * d;
				dpdx[k] = d;
				start[k] = s;
			}
		}
	}
	if (istartx >= istopx) istartx = istopx = 0;
}

// (model2_video.cpp's, from MAME)
uint LERP(uint x, uint y, uint a) { return (x + (((y - x) * a) >> 8)) & 0x00ff00ffu; }

// fast_log2's table, four entries a word, in the kernels' uniforms (k_log2: a private array would be
// copied into every thread's scratch memory on some GPUs)
uint log2_table(uint i) { return (k_log2[i >> 4][(i >> 2) & 3u] >> ((i & 3u) * 8u)) & 0xffu; }

int fast_log2(float value)
{
	if (value < 0.0f) return 0;
	const uint ival = asuint(value) >> 16;
	const int ex = int(ival >> 7) - 127;
	return (ex << 8) | int(log2_table(ival & 127u));
}

uint sheet_word(uint s, uint i) { return s ? LD(tex1, i) : LD(tex0, i); }

uint fetch_bilinear_texel(pinfo o, uint s0, uint s1, bool translucent, int miplevel, int u, int v)
{
	uint tex_width, tex_height, tex_x, tex_y, sheet;
	if (miplevel == -1) {
		// microtexture
		tex_width = 128; tex_height = 128;
		tex_x = o.utexx; tex_y = o.utexy;
		sheet = s1;
		u <<= 1u << o.utexminlod;
		v <<= 1u << o.utexminlod;
	} else {
		tex_width = o.texw >> uint(miplevel);
		tex_height = o.texh >> uint(miplevel);
		tex_x = ((o.texx - 2048u) >> uint(miplevel)) & 2047u;
		tex_y = ((o.texy - 1024u) >> uint(miplevel)) & 1023u;
		sheet = (miplevel & 1) ? s1 : s0;
		u >>= miplevel;
		v >>= miplevel;
	}
	if (o.mirrorx && (uint(u) & (tex_width << 8))) u = ~u;
	if (o.mirrory && (uint(v) & (tex_height << 8))) v = ~v;
	u -= 0x80;
	v -= 0x80;
	uint ufrac = uint(u) & 0xffu;
	uint vfrac = uint(v) & 0xffu;
	uint u0 = uint(u >> 8) & (tex_width - 1u);
	uint u1 = (u0 + 1u) & (tex_width - 1u);
	uint v0 = uint(v >> 8) & (tex_height - 1u);
	uint v1 = (v0 + 1u) & (tex_height - 1u);
	if (!o.wrapx && u1 == 0u) {
		if (ufrac >= 0x80u) { u0 = u1; u1++; ufrac = 0u; }
		else { u1 = u0; u0--; ufrac = 0x100u; }
	}
	if (!o.wrapy && v1 == 0u) {
		if (vfrac >= 0x80u) { v0 = 0u; v1++; vfrac = 0u; }
		else { v1 = v0; v0--; vfrac = 0x100u; }
	}
	// get_texel's addressing: sheets mapped 2048 x 1024, stored 1024 x 2048
	uint xo[2], fl[2], sh[2];
	[unroll] for (int k = 0; k < 2; k++) {
		const uint x = k ? u1 : u0;
		uint x2 = tex_x + x;
		const uint right = x2 >= 1024u ? 1024u : 0u;
		x2 -= right;
		xo[k] = x2 >> 1;
		fl[k] = right << 8;
		sh[k] = ((xo[k] & 1u) << 4) + ((~x & 1u) << 2);
	}
	const uint r0 = ((tex_y + v0) >> 1) << 9, r1 = ((tex_y + v1) >> 1) << 9;
	const uint ys0 = (~v0 & 1u) << 3, ys1 = (~v1 & 1u) << 3;
	uint tex00 = ((sheet_word(sheet, (((r0 ^ fl[0]) | xo[0]) >> 1) & 0x7ffffu) >> (sh[0] + ys0)) & 0x0fu) << 4;
	uint tex01 = ((sheet_word(sheet, (((r0 ^ fl[1]) | xo[1]) >> 1) & 0x7ffffu) >> (sh[1] + ys0)) & 0x0fu) << 4;
	uint tex10 = ((sheet_word(sheet, (((r1 ^ fl[0]) | xo[0]) >> 1) & 0x7ffffu) >> (sh[0] + ys1)) & 0x0fu) << 4;
	uint tex11 = ((sheet_word(sheet, (((r1 ^ fl[1]) | xo[1]) >> 1) & 0x7ffffu) >> (sh[1] + ys1)) & 0x0fu) << 4;
	if (translucent) {
		if (tex00 != 0xf0u) tex00 |= 0x00800000u;
		if (tex01 != 0xf0u) tex01 |= 0x00800000u;
		if (tex10 != 0xf0u) tex10 |= 0x00800000u;
		if (tex11 != 0xf0u) tex11 |= 0x00800000u;
		if (tex00 == 0x000000f0u) tex00 = tex01 & 0xffu;
		if (tex01 == 0x000000f0u) tex01 = tex00 & 0xffu;
		if (tex10 == 0x000000f0u) tex10 = tex11 & 0xffu;
		if (tex11 == 0x000000f0u) tex11 = tex10 & 0xffu;
	}
	uint tex0x = LERP(tex00, tex01, ufrac);
	uint tex1x = LERP(tex10, tex11, ufrac);
	if (translucent) {
		if (tex0x == 0x000000f0u) tex0x = tex1x & 0xffu;
		if (tex1x == 0x000000f0u) tex1x = tex0x & 0xffu;
	}
	return LERP(tex0x, tex1x, vfrac);
}

// draw_scanline_tex's texel at 1/z, u/z, v/z (q), before luma: the mip levels and microtexture blended
uint texel_at(pinfo o, float3 q)
{
	const bool translucent = (o.flags & 4u) != 0u;
	const uint s0 = (o.flags & 8u) ? 1u : 0u;
	const uint s1 = 1u - s0;
	precise const float z = fdiv(1.0f, q.x);
	const int mml = -o.texlod + fast_log2(z);
	const int level = clamp(mml >> 7, 0, o.max_level);
	precise const float uf = (q.y * z) * 256.0f;
	precise const float vf = (q.z * z) * 256.0f;
	const int u = ftoi(uf);
	const int v = ftoi(vf);
	uint tx = fetch_bilinear_texel(o, s0, s1, translucent, level, u, v);
	if (mml > 0 && level < o.max_level)
		tx = LERP(tx, fetch_bilinear_texel(o, s0, s1, translucent, level + 1, u, v), uint(mml & 127) << 1);
	else if (o.utex && mml < 0)
		tx = LERP(tx, fetch_bilinear_texel(o, s0, s1, translucent, -1, u, v), uint(min(-mml >> o.utexminlod, 127)));
	return tx;
}
#endif

// ---- the owners cleared (none: 0xffffffff) ----
#ifdef K_CLEAR
RWByteAddressBuffer owner : register(u0, space1);
cbuffer clear_k : register(b0, space2) { uint k_n; uint k_value; uint k_pad0; uint k_pad1; };
[numthreads(64, 1, 1)]
void main(uint3 t : SV_DispatchThreadID)
{
	const uint i = t.x + t.y * 65535u * 64u;
	if (i < k_n) ST(owner, i, k_value);
}
#endif

// ---- the rasterizer's spans, a thread a polygon's row ----
#ifdef K_SPANS
[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	const uint t = tid.x + tid.y * 65535u * 64u;
	if (t >= k_total) return;
	uint lo = 0, hi = k_jobs - 1;
	while (lo < hi) { const uint mid = (lo + hi + 1) >> 1; if (LD(jobbase, mid) <= t) lo = mid; else hi = mid - 1; }
	const uint idx = LD(jobpoly, lo);
	const pinfo o = poly_at(idx);
	const pverts pv = verts.Load<pverts>(idx * sizeof(pverts));
	const uint r = t - o.rowbase;
	const int y = o.y0 + int(r);
	int sx, stop;
	precise float3 q, d;
	extent(o, pv, y, o.clipx, o.clipx1, sx, stop, q, d);
	int step = 1;
	if (o.flags & 1u) {   // checker: the pixels with (x ^ y) odd, two at a time
		if (((sx ^ y) & 1) == 0) { sx++; q += d; }
		d *= 2.0f;
		step = 2;
	}
	rowrec rr;
	rr.sx = sx; rr.stop = stop; rr.d0 = d.x; rr.d1 = d.y; rr.d2 = d.z;
	rows.Store<rowrec>((r + o.rowbase) * sizeof(rowrec), rr);
	const bool tex = (o.flags & 2u) != 0u, see_through = (o.flags & 6u) == 6u;
	const uint orow = uint(y) * k_W;
	uint n = 0;
	for (int x = sx; x < stop; x += step, n++) {
		if (tex && (n % CK) == 0u && n / CK < o.ckstride) {
			const uint at = (o.ckbase + r * o.ckstride + n / CK) * 3u;
			ST(ck, at, asuint(q.x)); ST(ck, at + 1u, asuint(q.y)); ST(ck, at + 2u, asuint(q.z));
		}
		bool drawn = true;
		if (see_through && texel_at(o, q) < 0x00400000u) drawn = false;   // (a translucent texel under half: not drawn)
		if (drawn) owner.InterlockedMin((orow + uint(x)) * 4u, idx);
		q += d;
	}
}
#endif

// ---- each pixel its owner's colour, its parameters stepped on from the nearest kept ----
#ifdef K_SHADE
uint byte_at(uint w, uint i) { return (w >> ((i & 3u) * 8u)) & 0xffu; }
// the texel's colour: luma, colorxlat, gamma (0xffRRGGBB)
uint texel_colour(pinfo o, uint tx)
{
	if (o.flags & 4u) tx &= 0xffu;
	const uint li = (o.lumabase + (tx >> 1)) & 0x7fffu;
	const uint luma = min((byte_at(LD(luts, LUMA_AT + (li >> 2)), li) * o.luma) / 256u, 0x3fu);
	const uint ir = 0x0000u + (((o.pal >> 0) & 0x1fu) << 8) + luma;
	const uint ig = 0x2000u + (((o.pal >> 5) & 0x1fu) << 8) + luma;
	const uint ib = 0x4000u + (((o.pal >> 10) & 0x1fu) << 8) + luma;
	const uint r = (LD(luts, XLAT_AT + (ir >> 1)) >> ((ir & 1u) * 16u)) & 0xffu;
	const uint g = (LD(luts, XLAT_AT + (ig >> 1)) >> ((ig & 1u) * 16u)) & 0xffu;
	const uint b = (LD(luts, XLAT_AT + (ib >> 1)) >> ((ib & 1u) * 16u)) & 0xffu;
	return 0xff000000u | (byte_at(LD(luts, GAMMA_AT + (r >> 2)), r) << 16) | (byte_at(LD(luts, GAMMA_AT + (g >> 2)), g) << 8) | byte_at(LD(luts, GAMMA_AT + (b >> 2)), b);
}
[numthreads(8, 8, 1)]
void main(uint3 g : SV_DispatchThreadID)
{
	if (g.x >= k_W || g.y >= k_H) return;
	const uint at = g.y * k_W + g.x;
	const uint idx = LD(owner, at);
	if (idx == 0xffffffffu) { ST(outbuf, at, 0u); return; }
	const pinfo o = poly_at(idx);
	if (!(o.flags & 2u)) { ST(outbuf, at, o.colour); return; }
	const uint r = uint(int(g.y) - o.y0);
	const rowrec rr = rows.Load<rowrec>((o.rowbase + r) * sizeof(rowrec));
	const uint n = uint(int(g.x) - rr.sx) >> ((o.flags & 1u) ? 1u : 0u);
	const uint j = min(n / CK, o.ckstride - 1u);
	const uint ca = (o.ckbase + r * o.ckstride + j) * 3u;
	precise float3 q = float3(LDF(ck, ca), LDF(ck, ca + 1u), LDF(ck, ca + 2u));
	const float3 d = float3(rr.d0, rr.d1, rr.d2);
	for (uint i = j * CK; i < n; i++) q += d;
	ST(outbuf, at, texel_colour(o, texel_at(o, q)));
}
#endif

// ---- into the game's target (RGBA8: R the lowest byte) ----
#if defined(K_OUT) || defined(K_EDGE)
[[vk::image_format("rgba8")]] RWTexture2D<unorm float4> dst : register(u0, space1);
float4 unpack(uint c) { return float4(float((c >> 16) & 255u), float((c >> 8) & 255u), float(c & 255u), float(c >> 24)) / 255.0f; }
#endif

#ifdef K_OUT
ByteAddressBuffer src : register(t0, space0);
cbuffer out_k : register(b0, space2) { uint k_w; uint k_h; uint k_ss; uint k_pad0; };
// as it is, or each ss x ss block averaged (premultiplied: what isn't drawn adds nothing, its share drawn the alpha)
[numthreads(8, 8, 1)]
void main(uint3 g : SV_DispatchThreadID)
{
	if (g.x >= k_w || g.y >= k_h) return;
	const uint W = k_w * k_ss;
	if (k_ss == 1u) { dst[g.xy] = unpack(LD(src, g.y * W + g.x)); return; }
	uint r = 0, gg = 0, b = 0, a = 0;
	for (uint j = 0; j < k_ss; j++)
		for (uint i = 0; i < k_ss; i++) {
			const uint c = LD(src, (g.y * k_ss + j) * W + g.x * k_ss + i);
			if (!c) continue;
			r += (c >> 16) & 255u; gg += (c >> 8) & 255u; b += c & 255u; a += 255u;
		}
	const uint n = k_ss * k_ss;
	const uint a8 = (2u * a + n) / (2u * n);   // (the share drawn, rounded half up)
	dst[g.xy] = float4(float(r / n), float(gg / n), float(b / n), float(a8)) / 255.0f;
}
#endif

// Low: model2a::edge_smooth on the 3D at its scale, the picture beneath from the 2D (below_3d)
#ifdef K_EDGE
ByteAddressBuffer src : register(t0, space0);
ByteAddressBuffer bd : register(t1, space0);
ByteAddressBuffer mid : register(t2, space0);
cbuffer edge_k : register(b0, space2) { uint k_W; uint k_H; uint k_s; uint k_w; uint k_padpx; int k_bdx; int k_bdy; uint k_filter; };
// std::lround for x >= 0 (half away from zero)
int lround_pos(float x)
{
	precise const float f = floor(x);
	precise const float fr = x - f;
	return ftoi(f) + (fr >= 0.5f ? 1 : 0);
}
uint below_3d(int X, int Y)
{
	const int s = int(k_s), w = int(k_w), W = w * s, H = 384 * s, P = int(k_padpx), pw = w + 2 * P;
	const int x = X / s, y = Y / s;
	const uint m = LD(mid, y * w + x);
	if (m) return m;
	if (k_filter) {
		precise const float u = fdiv(float(X + k_bdx) + 0.5f, float(s)) - 0.5f;
		precise const float v = fdiv(float(Y + k_bdy) + 0.5f, float(s)) - 0.5f;
		const int ua = clamp(ftoi(floor(u)), 0, w - 1), va = clamp(ftoi(floor(v)), 0, 383);
		const int ub = clamp(ftoi(floor(u)) + 1, 0, w - 1), vb = clamp(ftoi(floor(v)) + 1, 0, 383);
		precise const float fu = u - floor(u), fv = v - floor(v);
		const uint c00 = LD(bd, (va + P) * pw + ua + P), c10 = LD(bd, (va + P) * pw + ub + P), c01 = LD(bd, (vb + P) * pw + ua + P), c11 = LD(bd, (vb + P) * pw + ub + P);
		uint o = 0xff000000u;
		for (int sh = 0; sh < 24; sh += 8) {
			precise const float t = float((c00 >> sh) & 255u) * (1 - fu) + float((c10 >> sh) & 255u) * fu;
			precise const float b = float((c01 >> sh) & 255u) * (1 - fu) + float((c11 >> sh) & 255u) * fu;
			precise const float tb = t * (1 - fv) + b * fv;
			o |= uint(lround_pos(tb)) << sh;
		}
		return o;
	}
	if (k_bdx || k_bdy) return LD(bd, (clamp(Y + k_bdy, 0, H - 1) / s + P) * pw + clamp(X + k_bdx, 0, W - 1) / s + P);
	return LD(bd, (y + P) * pw + x + P);
}
int luma_of(uint c) { return int((c >> 16) & 0xffu) * 77 + int((c >> 8) & 0xffu) * 150 + int(c & 0xffu) * 29; }
[numthreads(8, 8, 1)]
void main(uint3 g : SV_DispatchThreadID)
{
	if (g.x >= k_W || g.y >= k_H) return;
	const int X = int(g.x), Y = int(g.y), W = int(k_W);
	uint outc = LD(src, Y * W + X);
	if (Y >= 1 && Y < int(k_H) - 1 && X >= 1 && X < W - 1) {
		uint c3[5] = { LD(src, Y * W + X), LD(src, (Y - 1) * W + X), LD(src, (Y + 1) * W + X), LD(src, Y * W + X - 1), LD(src, Y * W + X + 1) };
		int l[5];
		bool any = false, all = true;
		[unroll] for (int i = 0; i < 5; i++) { l[i] = c3[i] ? luma_of(c3[i] & 0xffffffu) : -1; any = any || c3[i] != 0u; all = all && c3[i] != 0u; }
		const int TH = 32 * 256;
		bool go = any;
		uint c[5] = { 0u, 0u, 0u, 0u, 0u };
		if (go && all) {
			int lo = l[0], hi = l[0];
			[unroll] for (int i = 1; i < 5; i++) { lo = min(lo, l[i]); hi = max(hi, l[i]); }
			go = hi - lo >= TH;
			[unroll] for (int i2 = 0; i2 < 5; i2++) c[i2] = c3[i2] & 0xffffffu;
		} else if (go) {
			const int2 at[5] = { int2(X, Y), int2(X, Y - 1), int2(X, Y + 1), int2(X - 1, Y), int2(X + 1, Y) };
			[unroll] for (int i = 0; i < 5; i++) { c[i] = (c3[i] ? c3[i] : below_3d(at[i].x, at[i].y)) & 0xffffffu; l[i] = luma_of(c[i]); }
		}
		if (go) {
			const int lm = l[0], ln = l[1], ls = l[2], lw = l[3], le = l[4];
			int lo = lm, hi = lm;
			[unroll] for (int i = 1; i < 5; i++) { lo = min(lo, l[i]); hi = max(hi, l[i]); }
			if (hi - lo >= TH) {
				precise const float sub = min(1.0f, fdiv(float(abs(ln + ls + lw + le - 4 * lm)), float(4 * (hi - lo))));
				precise const float sm = sub * sub * (3.0f - 2.0f * sub);
				precise const float kf = sm * sm * (0.75f * 0.5f * 256.0f) + 0.5f;
				const int kk = ftoi(kf);
				if (kk) {
					const bool across_v = abs(ln + ls - 2 * lm) >= abs(lw + le - 2 * lm);
					const uint c1 = across_v ? c[1] : c[3], c2 = across_v ? c[2] : c[4];
					const int l1 = across_v ? ln : lw, l2 = across_v ? ls : le;
					const uint other = abs(l1 - lm) >= abs(l2 - lm) ? c1 : c2;
					uint m = 0xff000000u;
					for (int sh = 0; sh < 24; sh += 8) {
						const int p = int((c[0] >> sh) & 255u), q = int((other >> sh) & 255u);
						m |= uint(p + (((q - p) * kk + 128) >> 8)) << sh;
					}
					outc = m;
				}
			}
		}
	}
	dst[g.xy] = unpack(outc);
}
#endif
