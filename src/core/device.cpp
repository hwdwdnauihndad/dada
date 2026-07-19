#include "device.h"
#include "theme/colors.h"

bool gfx_create_shared(GfxShared& g) {
    D3D_FEATURE_LEVEL fl=D3D_FEATURE_LEVEL_11_0, got;
    HRESULT hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,0,0,&fl,1,D3D11_SDK_VERSION,&g.dev,&got,&g.ctx);
    if(FAILED(hr)) hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,0,0,&fl,1,D3D11_SDK_VERSION,&g.dev,&got,&g.ctx);
    return SUCCEEDED(hr)&&g.dev&&g.ctx;
}
void gfx_destroy_shared(GfxShared& g) { if(g.ctx){g.ctx->Release();g.ctx=nullptr;} if(g.dev){g.dev->Release();g.dev=nullptr;} }

bool gfx_create_surface(AppSurface& s, GfxShared& g, HWND hwnd, int w, int h) {
    s.hwnd=hwnd; s.w=w; s.h=h;
    IDXGIDevice* dxgiDev=nullptr; g.dev->QueryInterface(__uuidof(IDXGIDevice),(void**)&dxgiDev);
    if(!dxgiDev) return false;
    DCompositionCreateDevice(dxgiDev,__uuidof(IDCompositionDevice),(void**)&s.dcompDev);
    dxgiDev->Release();
    if(!s.dcompDev) return false;
    s.dcompDev->CreateTargetForHwnd(hwnd,TRUE,&s.dcompTarget);
    s.dcompDev->CreateVisual(&s.dcompVis);
    s.dcompTarget->SetRoot(s.dcompVis);

    IDXGIFactory2* f=nullptr; CreateDXGIFactory1(__uuidof(IDXGIFactory2),(void**)&f);
    if(!f) return false;
    DXGI_SWAP_CHAIN_DESC1 sd={}; sd.Width=w; sd.Height=h; sd.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count=1; sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount=2; sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    sd.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED; sd.Scaling=DXGI_SCALING_STRETCH;
    f->CreateSwapChainForComposition(g.dev,&sd,nullptr,&s.swap);
    f->Release();
    if(!s.swap) return false;
    s.dcompVis->SetContent(s.swap);
    s.dcompDev->Commit();
    gfx_resize_surface(s,g,w,h);
    return true;
}
void gfx_resize_surface(AppSurface& s, GfxShared& g, int w, int h) {
    s.w=w; s.h=h; if(s.rtv){s.rtv->Release();s.rtv=nullptr;}
    if(s.swap) s.swap->ResizeBuffers(2,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,0);
    ID3D11Texture2D* back=nullptr;
    if(s.swap) s.swap->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&back);
    if(back){ g.dev->CreateRenderTargetView(back,nullptr,&s.rtv); back->Release(); }
}
void gfx_destroy_surface(AppSurface& s) {
    if(s.rtv){s.rtv->Release();s.rtv=nullptr;}
    if(s.swap){s.swap->Release();s.swap=nullptr;}
    if(s.dcompVis){s.dcompVis->Release();s.dcompVis=nullptr;}
    if(s.dcompTarget){s.dcompTarget->Release();s.dcompTarget=nullptr;}
    if(s.dcompDev){s.dcompDev->Release();s.dcompDev=nullptr;}
}
void gfx_render_surface(AppSurface& s, GfxShared& g, ImDrawData* dd) {
    if(!s.rtv||!g.ctx) return;
    float clr[4]={colors::bg.x,colors::bg.y,colors::bg.z,0.f};
    g.ctx->OMSetRenderTargets(1,&s.rtv,nullptr);
    g.ctx->ClearRenderTargetView(s.rtv,clr);
    ImGui_ImplDX11_RenderDrawData(dd);
    if(s.swap) s.swap->Present(1,0);
    if(s.dcompDev) s.dcompDev->Commit();
}
