#include "main_page.h"
#include "theme/colors.h"
#include "theme/layout.h"
#include "helpers/widgets.h"
#include "config.h"
#include "game.h"
#include "esp.h"
#include <cstdio>

static const char* kTabs[]={"Aimbot","Visuals","Misc","Settings"};
static const char* kSnapeOrigins[]={"Bottom","Crosshair"};

static void draw_header(ImVec2 wp, ImVec2 ws, main_page::state& s) {
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 topEnd(ImVec2(wp.x+ws.x,wp.y+layout::topbar_h));
    dl->AddRectFilled(wp,topEnd,ImGui::GetColorU32(colors::topbar),layout::shell_round,ImDrawFlags_RoundCornersTop);

    dl->AddText(ImGui::GetFont(),16.f,ImVec2(wp.x+20,wp.y+18),ImGui::GetColorU32(ImVec4(1,1,1,0.7f)),"arc");

    ImVec2 tabArea(ImVec2(wp.x+ws.x/2-88,wp.y+10));
    for(int i=0;i<4;i++) {
        ImVec2 tp(tabArea.x+i*(layout::tab_w+6),tabArea.y), tsz(layout::tab_w,layout::tab_h);
        ImGui::SetCursorScreenPos(tp);
        char lid[32]; _snprintf(lid,32,"##tab%d",i);
        ImGui::InvisibleButton(lid,tsz);
        if(ImGui::IsItemClicked()) s.activeTab=i;
        bool sel=s.activeTab==i;
        float t=0; if(sel) t=1.f;
        if(sel) dl->AddRectFilled(tp,ImVec2(tp.x+tsz.x,tp.y+tsz.y),ImGui::GetColorU32(colors::surface),6.f);
        else if(ImGui::IsItemHovered()) dl->AddRectFilled(tp,ImVec2(tp.x+tsz.x,tp.y+tsz.y),ImGui::GetColorU32(colors::widget_hover),6.f);
        if(sel) dl->AddRectFilled(ImVec2(tp.x+4,tp.y+tsz.y-2),ImVec2(tp.x+tsz.x-4,tp.y+tsz.y),ImGui::GetColorU32(colors::accent),2.f);
        ImVec2 ts=ImGui::CalcTextSize(kTabs[i]);
        dl->AddText(ImVec2(tp.x+(tsz.x-ts.x)/2,tp.y+(tsz.y-ts.y)/2),ImGui::GetColorU32(sel?ImVec4(1,1,1,0.9f):ImVec4(1,1,1,0.4f)),kTabs[i]);
    }

    ImVec2 closeP(wp.x+ws.x-34,wp.y+16);
    ImGui::SetCursorScreenPos(closeP);
    if(ImGui::InvisibleButton("##close",ImVec2(22,22))) s.exitPressed=true;
    dl->AddLine(ImVec2(closeP.x+5,closeP.y+5),ImVec2(closeP.x+17,closeP.y+17),IM_COL32(255,255,255,100),1.5f);
    dl->AddLine(ImVec2(closeP.x+17,closeP.y+5),ImVec2(closeP.x+5,closeP.y+17),IM_COL32(255,255,255,100),1.5f);
}

static void draw_aimbot_tab(ImVec2 cp, ImVec2 avail, main_page::state& s) {
    ImGui::SetCursorScreenPos(ImVec2(cp.x+20,cp.y));
    ImGui::BeginChild("##aimbot",ImVec2(avail.x-40,avail.y),false,ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleColor(ImGuiCol_Text,colors::text_muted);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()+30);
    ImGui::Text("Aimbot features coming soon.");
    ImGui::PopStyleColor();
    ImGui::EndChild();
}

static void draw_visuals_tab(ImVec2 cp, ImVec2 avail, main_page::state& s) {
    ImGui::SetCursorScreenPos(ImVec2(cp.x+20,cp.y));
    ImGui::BeginChild("##visuals",ImVec2(avail.x-40,avail.y),false,ImGuiWindowFlags_NoBackground);

    section_header("MASTER");
    toggle_row("##espenable","Enable ESP",&g_Config.ESP_Enabled,s.blockDrag);
    ImGui::Spacing();

    if(g_Config.ESP_Enabled) {
        section_header("BOX OUTLINE");
        toggle_row("##espbox","Enable",&g_Config.ESP_Boxes,s.blockDrag);
        if(g_Config.ESP_Boxes){
            checkbox_row("##espcorner","Corner Mode",&g_Config.ESP_BoxCornerMode,s.blockDrag);
            slider_float_row("##boxthick","Thickness",&g_Config.ESP_BoxThickness,0.5f,4.f,"%.1f",s.blockDrag);
        }

        ImGui::Spacing();
        section_header("SKELETON OUTLINE");
        toggle_row("##espskel","Enable",&g_Config.ESP_Skeleton,s.blockDrag);
        if(g_Config.ESP_Skeleton){
            slider_float_row("##skelthick","Line Width",&g_Config.ESP_SkeletonThickness,0.5f,4.f,"%.1f",s.blockDrag);
        }

        ImGui::Spacing();
        section_header("OVERLAYS");
        toggle_row("##espsnap","Snaplines",&g_Config.ESP_Snaplines,s.blockDrag);
        if(g_Config.ESP_Snaplines) combo_row("##snaporigin","Origin",&g_Config.ESP_SnaplineOrigin,kSnapeOrigins,2,s.blockDrag);

        ImGui::Spacing();
        section_header("PLAYER INFO");
        toggle_row("##espname","Names",&g_Config.ESP_Names,s.blockDrag);
        toggle_row("##espdist","Distance",&g_Config.ESP_Distance,s.blockDrag);
        toggle_row("##esphealth","Health Bar",&g_Config.ESP_Health,s.blockDrag);
        toggle_row("##espweapon","Weapon",&g_Config.ESP_Weapon,s.blockDrag);
        toggle_row("##espheaddot","Head Dot",&g_Config.ESP_HeadDot,s.blockDrag);

        ImGui::Spacing();
        section_header("FILTERS");
        checkbox_row("##espshowlocal","Show Local Player",&g_Config.ESP_ShowLocal,s.blockDrag);
        checkbox_row("##espteam","Team Check",&g_Config.ESP_TeamCheck,s.blockDrag);
        slider_float_row("##espmaxdist","Max Distance (m)",&g_Config.ESP_MaxDistance,10.f,1000.f,"%.0f",s.blockDrag);
    }

    ImGui::EndChild();
}

static void draw_misc_tab(ImVec2 cp, ImVec2 avail, main_page::state& s) {
    ImGui::SetCursorScreenPos(ImVec2(cp.x+20,cp.y));
    ImGui::BeginChild("##misc",ImVec2(avail.x-40,avail.y),false,ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleColor(ImGuiCol_Text,colors::text_muted);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()+30);
    ImGui::Text("Misc features coming soon.");
    ImGui::PopStyleColor();
    ImGui::EndChild();
}

static void draw_settings_tab(ImVec2 cp, ImVec2 avail, main_page::state& s) {
    ImGui::SetCursorScreenPos(ImVec2(cp.x+20,cp.y));
    ImGui::BeginChild("##settings",ImVec2(avail.x-40,avail.y),false,ImGuiWindowFlags_NoBackground);
    section_header("CONFIG");
    ImGui::Spacing();
    GameData& gd=GameData::Get();
    ImGui::Text("Camera: %s",gd.m_Camera.Valid?"active":"searching...");
    ImGui::Text("Players: %d",gd.m_PlayerCount);
    ImGui::Text("FPS: %.0f",ImGui::GetIO().Framerate);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if(ImGui::Button("EJECT",ImVec2(ImGui::GetContentRegionAvail().x-20,36))) s.exitPressed=true;
    ImGui::EndChild();
}

void main_page::init(state& s) { s.activeTab=1; }

void main_page::draw(state& s) {
    s.blockDrag=false;
    ImGuiIO& io=ImGui::GetIO();
    float px=(io.DisplaySize.x-layout::panel_w)/2, py=(io.DisplaySize.y-layout::panel_h)/2;
    ImGui::SetNextWindowPos(ImVec2(px,py),ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(layout::panel_w,layout::panel_h),ImGuiCond_Once);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,layout::shell_round);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin(layout::win_id,nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings);
    ImVec2 wp=ImGui::GetWindowPos(), ws=ImGui::GetWindowSize();
    ImDrawList* dl=ImGui::GetWindowDrawList();
    dl->AddRectFilled(wp,ImVec2(wp.x+ws.x,wp.y+ws.y),ImGui::GetColorU32(colors::panel),layout::shell_round);
    dl->AddRect(wp,ImVec2(wp.x+ws.x,wp.y+ws.y),ImGui::GetColorU32(colors::divider),layout::shell_round);

    draw_header(wp,ws,s);

    ImVec2 cp(ImVec2(wp.x,wp.y+layout::topbar_h));
    ImVec2 avail(ImVec2(ws.x,ws.y-layout::topbar_h));
    switch(s.activeTab) {
        case 0: draw_aimbot_tab(cp,avail,s); break;
        case 1: draw_visuals_tab(cp,avail,s); break;
        case 2: draw_misc_tab(cp,avail,s); break;
        case 3: draw_settings_tab(cp,avail,s); break;
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}
