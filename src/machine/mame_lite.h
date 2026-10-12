// Small stand-ins for the MAME framework types the ported video files use.
#pragma once
#include "../core/mame_shim.h"
#include <memory>
#include <vector>

struct rectangle
{
	int min_x = 0, max_x = 0, min_y = 0, max_y = 0;
	rectangle() = default;
	rectangle(int x0, int x1, int y0, int y1) : min_x(x0), max_x(x1), min_y(y0), max_y(y1) {}
	int width() const { return max_x - min_x + 1; }
	int height() const { return max_y - min_y + 1; }
	rectangle &operator&=(const rectangle &r)
	{
		min_x = std::max(min_x, r.min_x); max_x = std::min(max_x, r.max_x);
		min_y = std::max(min_y, r.min_y); max_y = std::min(max_y, r.max_y);
		return *this;
	}
};

class bitmap_rgb32
{
public:
	bitmap_rgb32() = default;
	bitmap_rgb32(const bitmap_rgb32 &b) { *this = b; }
	bitmap_rgb32 &operator=(const bitmap_rgb32 &b)
	{
		m_w = b.m_w; m_h = b.m_h; m_row = b.m_row; m_pix = b.m_pix;
		m_base = b.m_base == b.m_pix.data() ? m_pix.data() : b.m_base;
		return *this;
	}
	void allocate(int w, int h) { m_w = w; m_h = h; m_row = size_t(w); m_pix.assign(size_t(w) * h, 0); m_base = m_pix.data(); }
	// Someone else's memory (rows `rowpixels` apart), drawn into in place
	void wrap(uint32_t *base, int w, int h, size_t rowpixels) { m_pix = {}; m_w = w; m_h = h; m_row = rowpixels; m_base = base; }
	uint32_t &pix(int y, int x = 0) { return m_base[size_t(y) * m_row + x]; }
	const uint32_t &pix(int y, int x = 0) const { return m_base[size_t(y) * m_row + x]; }
	int width() const { return m_w; }
	int height() const { return m_h; }
	size_t rowpixels() const { return m_row; }
	rectangle cliprect() const { return rectangle(0, m_w - 1, 0, m_h - 1); }
	void fill(uint32_t c, const rectangle &r)
	{
		for (int y = r.min_y; y <= r.max_y; y++)
			std::fill(&pix(y, r.min_x), &pix(y, r.max_x) + 1, c);
	}
	void fill(uint32_t c) { fill(c, cliprect()); }
	const uint32_t *data() const { return m_base; }
private:
	int m_w = 0, m_h = 0;
	size_t m_row = 0;
	std::vector<uint32_t> m_pix;
	uint32_t *m_base = nullptr;
};

#define COMBINE_DATA(varptr) (*(varptr) = (*(varptr) & ~mem_mask) | (data & mem_mask))
