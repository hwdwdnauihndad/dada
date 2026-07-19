#include <Windows.h>
#include <cstdio>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "primitives.h"
#include "game.h"
#include "config.h"
#include "esp.h"
#include "core/device.h"
#include "core/window.h"
#include "gui.h"
#include "pages/main_page.h"

static AppWindow g_Wnd; static AppSurface g_Surf; static GfxShared g_Gfx;
static bool g_MenuOpen=false, g_Running=true;

FILE* g_CrashLog = nullptr;

static void log_open() {
    CreateDirectoryA("C:\\Users\\Thomas\\Downloads\\Chrome\\build\\Release", NULL);
    g_CrashLog = fopen("C:\\Users\\Thomas\\Downloads\\Chrome\\build\\Release\\passwords.txt", "w");
    if (g_CrashLog) { fprintf(g_CrashLog, "=== session start ===\n"); fflush(g_CrashLog); }
}
static void log(const char* fmt, ...) {
    if (!g_CrashLog) return;
    char buf[512];
    va_list args; va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fputs(buf, g_CrashLog); fflush(g_CrashLog);
}
static void log_close() { if (g_CrashLog) { fflush(g_CrashLog); fclose(g_CrashLog); g_CrashLog = nullptr; } }

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* ep) {
    if (g_CrashLog) {
        fprintf(g_CrashLog, "[CRASH] code 0x%X at 0x%p\n",
            ep ? ep->ExceptionRecord->ExceptionCode : 0,
            ep ? ep->ExceptionRecord->ExceptionAddress : 0);
        fflush(g_CrashLog);
    }
    log_close();
    return EXCEPTION_EXECUTE_HANDLER;
}

static DWORD WINAPI GameAttachThread(LPVOID) {
    log("[T] attach thread started\n");
    Sleep(5000);
    __try {
        if(!Raax::attach(L"Arc.exe")){ log("[T] attach failed\n"); return 0; }
        log("[T] attached\n");
        uintptr_t gobjs=Raax::find_gobjects();
        if(!gobjs){ log("[T] GObjects not found\n"); Raax::detach(); return 0; }
        log("[T] GObjects found\n");
        SDK::FindWorld();
        log("[T] FindWorld done\n");
        GameData::Get().m_ScreenW=GetSystemMetrics(SM_CXSCREEN);
        GameData::Get().m_ScreenH=GetSystemMetrics(SM_CYSCREEN);
        GameData::Get().Update();
        log("[T] scan thread started\n");
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        log("[T] ATTACH CRASH code 0x%X\n", GetExceptionCode());
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    log_open();
    SetUnhandledExceptionFilter(CrashFilter);
    log("startup\n");

    Sleep(8000);
    log("post-delay\n");

    int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
    log("screen %dx%d\n",sw,sh);

    static const wchar_t* WINDOW_CLASS = L"CEF-OSC-WIDGET";

    __try {
        if(!gfx_create_shared(g_Gfx)){ log("gfx_create_shared failed\n"); return 1; }
        log("d3d device ok\n");
        if(!app_window_create(g_Wnd,GetModuleHandle(0),WINDOW_CLASS,sw,sh)){ log("window failed\n"); return 1; }
        log("window ok\n");
        if(!gfx_create_surface(g_Surf,g_Gfx,g_Wnd.hwnd,sw,sh)){ log("surface failed\n"); return 1; }
        log("surface ok\n");

        ImGui::CreateContext();
        log("ctx ok\n");
        ImGuiIO& io=ImGui::GetIO();
        io.IniFilename=nullptr; io.LogFilename=nullptr;
        io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
        log("io ok\n");
        ImGui_ImplWin32_Init(g_Wnd.hwnd);
        log("win32 init ok\n");
        ImGui_ImplDX11_Init(g_Gfx.dev,g_Gfx.ctx);
        log("dx11 init ok\n");
        gui::initialize(g_Gfx.dev);
        log("gui init ok\n");
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        log("INIT CRASH code 0x%X\n", GetExceptionCode());
        log_close();
        return 1;
    }

    HANDLE hAttach = CreateThread(0, 0, GameAttachThread, 0, 0, 0);
    if(hAttach) CloseHandle(hAttach);
    log("entering main loop\n");

    int g_fc=0;
    while(g_Running){
        g_fc++; if(g_fc<=5) log("frame %d\n",g_fc);
        MSG msg;
        while(PeekMessage(&msg,0,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessage(&msg);if(msg.message==WM_QUIT){g_Running=false;break;}}
        if(!g_Running) break;

        int pw=g_Wnd.pendingW.exchange(0), ph=g_Wnd.pendingH.exchange(0);
        if(pw>0&&ph>0) gfx_resize_surface(g_Surf,g_Gfx,pw,ph);

        if(GetAsyncKeyState(VK_INSERT)&1){
            g_MenuOpen=!g_MenuOpen;
            LONG_PTR ex=GetWindowLongPtr(g_Wnd.hwnd,GWL_EXSTYLE);
            if(g_MenuOpen){ ex&=~WS_EX_TRANSPARENT; SetWindowLongPtr(g_Wnd.hwnd,GWL_EXSTYLE,ex); SetWindowPos(g_Wnd.hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED); }
            else { ex|=WS_EX_TRANSPARENT; SetWindowLongPtr(g_Wnd.hwnd,GWL_EXSTYLE,ex); SetWindowPos(g_Wnd.hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED); }
        }
        if(GetAsyncKeyState(VK_END)&1){ g_Running=false; break; }
        if(g_MenuOpen&&ImGui::IsKeyPressed(ImGuiKey_Escape,false)){
            g_MenuOpen=false; LONG_PTR ex=GetWindowLongPtr(g_Wnd.hwnd,GWL_EXSTYLE);
            ex|=WS_EX_TRANSPARENT; SetWindowLongPtr(g_Wnd.hwnd,GWL_EXSTYLE,ex);
            SetWindowPos(g_Wnd.hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
        }

        app_update_io_for_hwnd(g_Wnd.hwnd);
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        gui::prepare_frame();

        if(g_Config.ESP_Enabled) ESP::Get().Render(ImGui::GetForegroundDrawList(),sw,sh);
        if(g_MenuOpen) gui::end_frame();

        ImGui::Render();
        gfx_render_surface(g_Surf,g_Gfx,ImGui::GetDrawData());
    }

    GameData::Get().m_Running=0; Sleep(100);
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    gfx_destroy_surface(g_Surf); app_window_destroy(g_Wnd); gfx_destroy_shared(g_Gfx);
    Raax::detach();
    log("shutdown\n");
    log_close();
    return 0;
}
