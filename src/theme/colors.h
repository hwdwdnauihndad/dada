#pragma once
#include "imgui/imgui.h"

namespace colors {
    inline constexpr ImVec4 accent{0.486f,0.227f,0.929f,1.f};
    inline constexpr ImVec4 accent_hover{0.556f,0.297f,0.989f,1.f};
    inline constexpr ImVec4 bg{0.031f,0.031f,0.039f,1.f};
    inline constexpr ImVec4 panel{0.055f,0.055f,0.067f,1.f};
    inline constexpr ImVec4 topbar{0.047f,0.047f,0.059f,1.f};
    inline constexpr ImVec4 widget{0.075f,0.075f,0.090f,1.f};
    inline constexpr ImVec4 widget_hover{0.098f,0.098f,0.118f,1.f};
    inline constexpr ImVec4 surface{0.063f,0.063f,0.075f,1.f};
    inline constexpr ImVec4 surface_hover{0.086f,0.086f,0.102f,1.f};
    inline constexpr ImVec4 text{0.89f,0.89f,0.89f,1.f};
    inline constexpr ImVec4 text_muted{0.42f,0.42f,0.42f,1.f};
    inline constexpr ImVec4 divider{1.f,1.f,1.f,0.05f};
    inline constexpr ImVec4 danger{0.92f,0.25f,0.25f,1.f};
    inline constexpr ImVec4 module_box{0.07f,0.07f,0.08f,0.7f};
    inline void sync_accent_hover() {}
}
