#pragma once
#include <Windows.h>
#include "imgui/imgui.h"

class ESP {
public:
    static ESP& Get() { static ESP e; return e; }
    void Render(ImDrawList* dl, int sw, int sh);
};
