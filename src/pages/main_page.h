#pragma once
#include "imgui/imgui.h"
namespace main_page {
    struct state {
        int activeTab=0; bool exitPressed=false, showPreview=true;
        bool blockDrag=false;
    };
    void init(state& s);
    void draw(state& s);
}
