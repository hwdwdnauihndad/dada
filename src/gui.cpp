#include "gui.h"
#include "core/device.h"
#include "theme/colors.h"
#include "theme/layout.h"
#include "pages/main_page.h"

static main_page::state g_mp;

void gui::initialize(ID3D11Device* dev) {
    apply_style();
    main_page::init(g_mp);
}
void gui::apply_style() {
    ImGuiStyle& s=ImGui::GetStyle(); ImVec4* c=s.Colors;
    s.WindowRounding=0; s.ChildRounding=8.f; s.FrameRounding=6.f; s.GrabRounding=6.f;
    s.ScrollbarRounding=6.f; s.PopupRounding=10.f; s.TabRounding=8.f;
    s.WindowBorderSize=0; s.FrameBorderSize=0; s.PopupBorderSize=0;
    s.WindowPadding=ImVec2(0,0); s.FramePadding=ImVec2(10,6);
    s.ItemSpacing=ImVec2(10,6); s.ScrollbarSize=4.f;
    c[ImGuiCol_Text]=colors::text; c[ImGuiCol_TextDisabled]=colors::text_muted;
    c[ImGuiCol_WindowBg]=ImVec4(0,0,0,0); c[ImGuiCol_ChildBg]=colors::panel;
    c[ImGuiCol_PopupBg]=ImVec4(0.08f,0.08f,0.09f,0.96f);
    c[ImGuiCol_Border]=colors::divider; c[ImGuiCol_FrameBg]=colors::widget;
    c[ImGuiCol_FrameBgHovered]=colors::widget_hover; c[ImGuiCol_FrameBgActive]=colors::surface_hover;
    c[ImGuiCol_TitleBg]=colors::topbar; c[ImGuiCol_TitleBgActive]=colors::topbar;
    c[ImGuiCol_Button]=colors::widget; c[ImGuiCol_ButtonHovered]=colors::widget_hover;
    c[ImGuiCol_ButtonActive]=colors::surface_hover;
    c[ImGuiCol_Header]=colors::widget; c[ImGuiCol_HeaderHovered]=colors::widget_hover;
    c[ImGuiCol_HeaderActive]=colors::surface_hover;
    c[ImGuiCol_Separator]=colors::divider; c[ImGuiCol_ScrollbarBg]=ImVec4(0,0,0,0);
    c[ImGuiCol_ScrollbarGrab]=ImVec4(1,1,1,0.1f); c[ImGuiCol_ScrollbarGrabHovered]=ImVec4(1,1,1,0.2f);
    c[ImGuiCol_ScrollbarGrabActive]=ImVec4(1,1,1,0.3f);
    c[ImGuiCol_CheckMark]=colors::accent; c[ImGuiCol_SliderGrab]=colors::accent;
    c[ImGuiCol_Tab]=colors::widget; c[ImGuiCol_TabHovered]=colors::widget_hover;
    c[ImGuiCol_TabSelected]=colors::surface; c[ImGuiCol_TabSelectedOverline]=colors::accent;
    c[ImGuiCol_TabDimmed]=colors::panel; c[ImGuiCol_TabDimmedSelected]=colors::surface;
    c[ImGuiCol_ResizeGrip]=colors::accent; c[ImGuiCol_TextSelectedBg]=ImVec4(0.31f,0.56f,0.97f,0.35f);
    c[ImGuiCol_NavCursor]=colors::accent;
}
void gui::prepare_frame() {
    ImGuiIO& io=ImGui::GetIO();
    float fw=(float)(int)io.DisplaySize.x, fh=(float)(int)io.DisplaySize.y;
    if(fw<100)fw=1920; if(fh<100)fh=1080;
    io.DisplaySize=ImVec2(fw,fh);
}
void gui::begin_frame() {}
void gui::end_frame() { main_page::draw(g_mp); }
