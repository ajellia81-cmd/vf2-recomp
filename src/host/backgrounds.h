// The widescreen sides painted for 4:3 screens (model2a::load_screen): <dir>/select.png (or .jpg),
// the player select at any size whose height is the screen's (the 4:3 in its middle, unused).
// Returns how many were found.
#pragma once
#include "../machine/model2a.h"
#include "../../third_party/stb/stb_image.h"
#include <cstdio>
#include <string>
#include <vector>

inline int load_backgrounds(model2a &m, const std::string &dir)
{
	int n = 0;
	for (const std::string &name : model2a::screen_names())
		for (const char *ext : { ".png", ".jpg", ".jpeg" }) {
			const std::string path = dir + "/" + name + ext;
			int w = 0, h = 0, comp = 0;
			unsigned char *px = stbi_load(path.c_str(), &w, &h, &comp, 4);
			if (!px) continue;
			std::vector<uint32_t> argb(size_t(w) * h);
			for (size_t i = 0; i < argb.size(); i++)
				argb[i] = uint32_t(px[4 * i + 3]) << 24 | uint32_t(px[4 * i]) << 16 | uint32_t(px[4 * i + 1]) << 8 | px[4 * i + 2];
			stbi_image_free(px);
			if (m.load_screen(name, std::move(argb), w, h)) {
				fprintf(stderr, "background: %s (%dx%d)\n", path.c_str(), w, h);
				n++;
			}
			break;
		}
	return n;
}
