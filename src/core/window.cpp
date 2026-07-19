#include "window.h"
#include "imgui/imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static AppWindow* g_appWnd=nullptr;

LRESULT CALLBACK app_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wp,lp)) return 1;
    if(!g_appWnd) return DefWindowProc(hwnd,msg,wp,lp);
    switch(msg) {
        case WM_SIZE: g_appWnd->pendingW.store(LOWORD(lp)); g_appWnd->pendingH.store(HIWORD(lp)); break;
        case WM_DESTROY: if(g_appWnd->quitOnDestroy) PostQuitMessage(0); break;
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}
bool app_window_create(AppWindow& aw, HINSTANCE hi, const wchar_t* cls, int wi, int he) {
    WNDCLASSEXW wc={sizeof(wc),CS_HREDRAW|CS_VREDRAW,app_wnd_proc,0,0,hi,LoadIconW(0,IDI_APPLICATION),LoadCursorW(0,IDC_ARROW),0,cls,cls};
    RegisterClassExW(&wc);
    aw.hwnd=CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT,cls,L"",WS_POPUP,0,0,wi,he,0,0,hi,0);
    if(!aw.hwnd) return false;
    g_appWnd=&aw; aw.quitOnDestroy=true;
    ShowWindow(aw.hwnd,SW_SHOW); return true;
}
void app_window_destroy(AppWindow& aw) {
    if(aw.hwnd){ DestroyWindow(aw.hwnd); aw.hwnd=nullptr; }
    g_appWnd=nullptr;
}
void app_update_io_for_hwnd(HWND hwnd) {
    ImGuiIO& io=ImGui::GetIO();
    RECT r; if(GetClientRect(hwnd,&r)){ io.DisplaySize.x=(float)(r.right-r.left); io.DisplaySize.y=(float)(r.bottom-r.top); }
    POINT cp; GetCursorPos(&cp); ScreenToClient(hwnd,&cp);
    io.AddMousePosEvent((float)cp.x,(float)cp.y);
    for(int b=0;b<5;b++) io.AddMouseButtonEvent(b,(GetAsyncKeyState(VK_LBUTTON+b)&0x8000)!=0);
}
