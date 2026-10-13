// A streaming texture of SDL's Direct3D 11 renderer written straight into, for send_rows
// (gpu_picture.h). SDL's lock and update make a new staging texture each time (a fresh
// allocation of the picture's size, 12 MB at 4x), then copy it into the texture on the GPU: a
// streaming texture there is a dynamic one, whose own way in is Map with WRITE_DISCARD (the
// driver hands out memory from its own pool, while the GPU may still read the last contents).
// The whole texture's contents are undefined after it but for the rows written.
#pragma once
#include <SDL3/SDL.h>

// The texture's memory (its rows pitch bytes apart), or false: not SDL's Direct3D 11 renderer, not
// a dynamic texture, or the map failed (send_rows then goes SDL's way). VF2_D3D11_MAP=0: never.
bool d3d11_map(SDL_Texture *t, void **pixels, int *pitch);
void d3d11_unmap(SDL_Texture *t);
