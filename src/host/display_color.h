#pragma once
#include <SDL3/SDL.h>
#include <algorithm>

// Modulate only the completed game picture. 100% preserves its original RGB;
// values above 100% multiply RGB before SDR clipping and keep black at black.
inline bool draw_game_picture(SDL_Renderer *renderer, SDL_Texture *picture, int brightness)
{
	if (!picture) return false;
	const float gain = std::clamp(brightness, 50, 200) / 100.0f;
	if (!SDL_SetTextureColorModFloat(picture, gain, gain, gain)) return false;
	const bool ok = SDL_RenderTexture(renderer, picture, nullptr, nullptr);
	SDL_SetTextureColorModFloat(picture, 1, 1, 1);
	return ok;
}
