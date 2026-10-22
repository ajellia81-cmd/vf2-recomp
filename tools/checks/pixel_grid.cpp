// No ROM or SDL required: integer compatibility and native raster-grid contracts.
#include "../../src/gpu/pixel_grid.h"
#include <cassert>
#include <limits>
int main()
{
	const gpu3d_pixel_grid legacy(4);
	assert(legacy.valid() && legacy.x == 4 && legacy.y == 4);
	assert(legacy.left(9, 1024) == 36 && legacy.right(9, 1024) == 40);
	const gpu3d_pixel_grid native(1, 2868.0f / 808, 1320.0f / 384);
	assert(native.valid());
	assert(native.left(202, 2868) == 717 && native.right(605, 2868) == 2151);
	assert(native.top(96, 1320) == 330 && native.bottom(287, 1320) == 990);
	assert(std::abs(808 * native.x - 2868) < .001f);
	assert(384 * native.y == 1320);
	assert(native.x != native.y);
	assert(native.left(-10, 2868) == 0 && native.right(1000, 2868) == 2868);
	assert(!gpu3d_pixel_grid(1, 0, 2).valid());
	assert(!gpu3d_pixel_grid(1, -1, 2).valid());
	assert(!gpu3d_pixel_grid(1, std::numeric_limits<float>::infinity(), 2).valid());
}
