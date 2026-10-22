#pragma once
#include "../gpu/gpu3d.h"

namespace ios_video {
inline constexpr int WIDTH = 2868, HEIGHT = 1320;
inline constexpr float ASPECT = float(WIDTH) / HEIGHT;
// The 2D layers retain their arcade coordinates. The 3D is rasterized directly
// at the requested resolution, correcting the arcade's non-square pixels.
inline void configure(gpu3d_frame &frame, int arcade_width, int arcade_height)
{
	frame.w = WIDTH; frame.h = HEIGHT;
	frame.scale = 1; frame.ss = 1; frame.edge = false;
	frame.pixel_scale_x = float(WIDTH) / arcade_width;
	frame.pixel_scale_y = float(HEIGHT) / arcade_height;
}
} // namespace ios_video
