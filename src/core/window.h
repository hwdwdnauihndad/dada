#pragma once
#include <Windows.h>
#include <atomic>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

struct AppWindow {
    HWND hwnd=nullptr; std::atomic<int> pendingW{0}, pendingH{0}; bool quitOnDestroy=false;
};
bool app_window_create(AppWindow& aw, HINSTANCE hi, const wchar_t* cls, int wi, int he);
void app_window_destroy(AppWindow& aw);
void app_update_io_for_hwnd(HWND hwnd);
LRESULT CALLBACK app_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
