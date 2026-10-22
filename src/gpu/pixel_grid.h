#pragma once
#include <algorithm>
#include <cmath>

// Pixel coordinates and clip windows on the actual rasterization grid. Zero
// overrides preserve desktop's integer scale; both overrides are required together.
struct gpu3d_pixel_grid {
	float x, y;
	gpu3d_pixel_grid(int scale, float sx = 0, float sy = 0)
		: x(sx == 0 && sy == 0 ? float(scale) : sx),
		  y(sx == 0 && sy == 0 ? float(scale) : sy) {}
	bool valid() const { return std::isfinite(x) && std::isfinite(y) && x > 0 && y > 0; }
	int left(int column, int width) const { return std::clamp(int(std::floor(column * x)), 0, width); }
	int right(int column, int width) const { return std::clamp(int(std::ceil((column + 1) * x)), 0, width); }
	int top(int row, int height) const { return std::clamp(int(std::floor(row * y)), 0, height); }
	int bottom(int row, int height) const { return std::clamp(int(std::ceil((row + 1) * y)), 0, height); }
};
