#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <functional>

struct RowCtx { ImVec2 rMin, rMax; bool& blockDrag; };
struct AnimSlot { float hover=0,fill=0,open=0; ImGuiID id=0; };
inline float smooth(float& v, float t, float sp=14.f) { float dt=ImGui::GetIO().DeltaTime; float step=sp*dt; if(step>1.f)step=1.f; v+=(t-v)*step; return v; }

bool toggle_row(const char* id, const char* label, bool* val, bool& blockDrag);
bool checkbox_row(const char* id, const char* label, bool* val, bool& blockDrag);
void section_header(const char* label);
bool combo_row(const char* id, const char* label, int* idx, const char* items[], int cnt, bool& blockDrag);
bool slider_float_row(const char* id, const char* label, float* val, float mn, float mx, const char* fmt, bool& blockDrag);
