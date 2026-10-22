// Synthetic polygons only. Never opens or uses a game archive.
#include "../../src/gpu/gpu3d.h"
#import <Metal/Metal.h>
#include <cstdio>
#include <vector>
int main()
{
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	if (!device) { std::puts("SKIP: No Metal device available on this runner"); return 77; }
	auto gpu = gpu3d_create_metal((__bridge void *)device);
	if (!gpu) { std::puts("FAIL: Metal shader initialization"); return 1; }
	constexpr int W = 2868, H = 1320;
	if (!gpu->make_targets(W, H, 1)) return 1;
	gpu3d_poly p{};
	p.n = 4; p.colour = 0xff336699;
	p.x[0] = p.x[3] = 202; p.x[1] = p.x[2] = 606;
	p.y[0] = p.y[1] = 96; p.y[2] = p.y[3] = 288;
	p.vp[0] = 0; p.vp[1] = 807; p.vp[2] = 0; p.vp[3] = 383;
	gpu3d_frame frame{};
	frame.w = W; frame.h = H;
	frame.pixel_scale_x = float(W) / 808;
	frame.pixel_scale_y = float(H) / 384;
	std::vector<uint32_t> pixels(size_t(W) * H);
	for (auto quality : {gpu3d_quality::exact, gpu3d_quality::fast}) {
		gpu->set_quality(quality);
		if (!gpu->submit(&p, 1, frame, 0) || !gpu->finish() || !gpu->read_target(0, pixels.data(), W)) return 2;
		if (pixels[size_t(H / 2) * W + W / 2] != p.colour || pixels[0] != 0) return 3;
		for (int x : {716, 718, 2149, 2152}) {
			const uint32_t wanted = x > 717 && x < 2151 ? p.colour : 0;
			if (pixels[size_t(660) * W + x] != wanted) return 4;
		}
		for (int y : {329, 331, 988, 991}) {
			const uint32_t wanted = y > 330 && y < 990 ? p.colour : 0;
			if (pixels[size_t(y) * W + 1434] != wanted) return 5;
		}
	}
	std::puts("PASS: exact and fast Metal rasterization at 2868x1320, using synthetic geometry only");
}
