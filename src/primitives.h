#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <stdint.h>

namespace Raax {

inline HANDLE g_Process = nullptr;
inline DWORD  g_Pid = 0;

inline int attach(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snap, &pe)) {
        do { if (!_wcsicmp(pe.szExeFile, name)) { g_Pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    if (!g_Pid) return 0;
    g_Process = OpenProcess(PROCESS_VM_READ|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, g_Pid);
    if (!g_Process) { g_Pid = 0; return 0; }
    return 1;
}

inline void detach() { if (g_Process) { CloseHandle(g_Process); g_Process = nullptr; } }
inline int alive() { if (!g_Process) return 0; DWORD c; return GetExitCodeProcess(g_Process, &c) && c == STILL_ACTIVE; }
inline int rpm(uintptr_t a, void* o, size_t s) {
    if (a < 0x10000 || a > 0x7FFFFFFFFFFFULL) return 0;
    SIZE_T rd = 0;
    if (!ReadProcessMemory(g_Process, (LPCVOID)a, o, s, &rd)) return 0;
    return (int)(rd == s);
}

template<typename T> T read(uintptr_t a) { T v{}; rpm(a, &v, sizeof(T)); return v; }

enum { SAFE_PROC = 1 };
static inline int safe_proc(uintptr_t a) {
    if (a < 0x10000 || a > 0x7FFFFFFFFFFFULL) return 0;
    MEMORY_BASIC_INFORMATION m{};
    if (!VirtualQueryEx(g_Process, (LPCVOID)a, &m, sizeof(m))) return 0;
    if (!(m.State == MEM_COMMIT)) return 0;
    if (m.Protect & (PAGE_GUARD | PAGE_NOACCESS)) return 0;
    return 1;
}

struct MemRegion { uintptr_t Base; size_t Size; DWORD Protect; };

inline int enum_regions(MemRegion* out, int max) {
    int c = 0; uintptr_t a = 0; MEMORY_BASIC_INFORMATION m;
    while (VirtualQueryEx(g_Process, (LPCVOID)a, &m, sizeof(m)) && c < max) {
        if (m.State == MEM_COMMIT && (m.Protect & (PAGE_READONLY|PAGE_READWRITE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE)))
            out[c++] = {(uintptr_t)m.BaseAddress, m.RegionSize, m.Protect};
        a = (uintptr_t)m.BaseAddress + m.RegionSize;
    }
    return c;
}

// GObjects: 48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1
inline uintptr_t find_gobjects() {
    static uintptr_t cached_gobjects = 0;
    if (cached_gobjects) return cached_gobjects;
    const uint8_t pat[] = {0x48,0x8B,0x05,0,0,0,0,0x48,0x8B,0x0C,0xC8,0x48,0x8D,0x04,0xD1};
    size_t plen = 14;

    HANDLE heap = GetProcessHeap();
    uintptr_t a = 0; MEMORY_BASIC_INFORMATION m;
    while (VirtualQueryEx(g_Process, (LPCVOID)a, &m, sizeof(m))) {
        if (!(m.State == MEM_COMMIT && (m.Protect & (PAGE_READONLY|PAGE_READWRITE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE)))) {
            a = (uintptr_t)m.BaseAddress + m.RegionSize; continue;
        }
        if (m.RegionSize < plen) { a = (uintptr_t)m.BaseAddress + m.RegionSize; continue; }
        size_t sz = m.RegionSize;
        if (sz > 0x8000000) sz = 0x8000000;
        uint8_t* buf = (uint8_t*)HeapAlloc(heap, 0, sz);
        if (!buf || !rpm((uintptr_t)m.BaseAddress, buf, sz)) {
            if (buf) HeapFree(heap, 0, buf);
            a = (uintptr_t)m.BaseAddress + m.RegionSize; continue;
        }
        for (size_t i = 0; i + plen <= sz; i++) {
            int match = 1;
            for (size_t j = 0; j < plen; j++)
                if (pat[j] && buf[i+j] != pat[j]) { match = 0; break; }
            if (match) {
                int32_t rel;
                if (rpm((uintptr_t)m.BaseAddress + i + 3, &rel, 4)) {
                    HeapFree(heap, 0, buf);
                    cached_gobjects = (uintptr_t)m.BaseAddress + i + 7 + rel;
                    return cached_gobjects;
                }
            }
        }
        HeapFree(heap, 0, buf);
        a = (uintptr_t)m.BaseAddress + m.RegionSize;
    }
    return 0;
}

} // namespace Raax
