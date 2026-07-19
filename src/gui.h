#pragma once
#include <d3d11.h>
#include "imgui/imgui.h"
namespace gui {
    void initialize(ID3D11Device* dev);
    void apply_style();
    void prepare_frame();
    void begin_frame();
    void end_frame();
}
