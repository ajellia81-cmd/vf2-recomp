// A streaming texture of SDL's Direct3D 11 renderer written straight into (d3d11_map.h).
#include "d3d11_map.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>

namespace {
ID3D11Texture2D *dynamic_texture(SDL_Texture *t, ID3D11DeviceContext **ctx)
{
	static const bool off = getenv("VF2_D3D11_MAP") && atoi(getenv("VF2_D3D11_MAP")) == 0;
	SDL_Renderer *ren = off || !t ? nullptr : SDL_GetRendererFromTexture(t);
	const char *name = ren ? SDL_GetRendererName(ren) : nullptr;
	if (!name || strcmp(name, "direct3d11") != 0) return nullptr;
	auto *dev = static_cast<ID3D11Device *>(SDL_GetPointerProperty(SDL_GetRendererProperties(ren), SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr));
	auto *tex = static_cast<ID3D11Texture2D *>(SDL_GetPointerProperty(SDL_GetTextureProperties(t), SDL_PROP_TEXTURE_D3D11_TEXTURE_POINTER, nullptr));
	if (!dev || !tex) return nullptr;
	D3D11_TEXTURE2D_DESC d;
	tex->GetDesc(&d);
	if (d.Usage != D3D11_USAGE_DYNAMIC || !(d.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) || d.MipLevels != 1 || d.ArraySize != 1) return nullptr;
	if (ctx) {
		SDL_FlushRenderer(ren);   // (SDL's queued drawing with the texture's last contents goes first)
		dev->GetImmediateContext(ctx);
		if (!*ctx) return nullptr;
	}
	return tex;
}
}

bool d3d11_map(SDL_Texture *t, void **pixels, int *pitch)
{
	ID3D11DeviceContext *ctx = nullptr;
	ID3D11Texture2D *tex = dynamic_texture(t, &ctx);
	if (!tex) return false;
	D3D11_MAPPED_SUBRESOURCE m;
	const bool ok = SUCCEEDED(ctx->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) && m.pData;
	ctx->Release();
	if (!ok) {
		static bool told = false;
		if (!told) { told = true; fprintf(stderr, "Direct3D 11: a dynamic texture couldn't be mapped: SDL's updates instead\n"); }
		return false;
	}
	static bool said = false;
	if (!said) { said = true; fprintf(stderr, "Direct3D 11: pictures sent up into the textures mapped (WRITE_DISCARD)\n"); }
	*pixels = m.pData;
	*pitch = int(m.RowPitch);
	return true;
}

void d3d11_unmap(SDL_Texture *t)
{
	ID3D11DeviceContext *ctx = nullptr;
	SDL_Renderer *ren = SDL_GetRendererFromTexture(t);
	auto *dev = ren ? static_cast<ID3D11Device *>(SDL_GetPointerProperty(SDL_GetRendererProperties(ren), SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr)) : nullptr;
	auto *tex = static_cast<ID3D11Texture2D *>(SDL_GetPointerProperty(SDL_GetTextureProperties(t), SDL_PROP_TEXTURE_D3D11_TEXTURE_POINTER, nullptr));
	if (!dev || !tex) return;
	dev->GetImmediateContext(&ctx);
	if (!ctx) return;
	ctx->Unmap(tex, 0);
	ctx->Release();
}
#else
bool d3d11_map(SDL_Texture *, void **, int *) { return false; }
void d3d11_unmap(SDL_Texture *) {}
#endif
