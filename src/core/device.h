#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dcomp.lib")

struct GfxShared { ID3D11Device* dev=nullptr; ID3D11DeviceContext* ctx=nullptr; };
bool gfx_create_shared(GfxShared& g);
void gfx_destroy_shared(GfxShared& g);

struct AppSurface {
    HWND hwnd=nullptr; IDXGISwapChain1* swap=nullptr; ID3D11RenderTargetView* rtv=nullptr;
    IDCompositionDevice* dcompDev=nullptr; IDCompositionTarget* dcompTarget=nullptr;
    IDCompositionVisual* dcompVis=nullptr; int w=0,h=0;
};
bool gfx_create_surface(AppSurface& s, GfxShared& g, HWND hwnd, int w, int h);
void gfx_resize_surface(AppSurface& s, GfxShared& g, int w, int h);
void gfx_destroy_surface(AppSurface& s);
void gfx_render_surface(AppSurface& s, GfxShared& g, ImDrawData* dd);
