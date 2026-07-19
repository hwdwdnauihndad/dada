#include "widgets.h"
#include "theme/colors.h"
#include "theme/layout.h"

static AnimSlot& get_anim(ImGuiID id) {
    static AnimSlot slots[512]; static int n=0;
    for(int i=0;i<n;i++) if(slots[i].id==id) return slots[i];
    if(n<512){ slots[n].id=id; return slots[n++]; }
    return slots[0];
}

bool toggle_row(const char* id, const char* label, bool* val, bool& blockDrag) {
    float rowH=layout::row_h; ImGuiID iid=ImGui::GetID(id);
    ImVec2 cp=ImGui::GetCursorScreenPos(), avail=ImGui::GetContentRegionAvail();
    ImVec2 rowMax(ImVec2(cp.x+avail.x,cp.y+rowH));
    ImGui::GetWindowDrawList()->AddRectFilled(cp,rowMax,IM_COL32(255,255,255,0),6.f);
    auto& a=get_anim(iid); smooth(a.hover,ImGui::IsMouseHoveringRect(cp,rowMax)?1.f:0.f,14.f);
    if(a.hover>0.01f) {
        ImGui::GetWindowDrawList()->AddRectFilled(cp,rowMax,IM_COL32(255,255,255,(int)(a.hover*10.f)),6.f);
        blockDrag=true;
    }
    ImVec2 tp(ImVec2(cp.x+avail.x-62,cp.y+(rowH-22)/2)), tsz(38,22);
    ImGui::SetCursorScreenPos(tp);
    ImGui::InvisibleButton(id,tsz);
    if(ImGui::IsItemClicked()) *val=!*val;
    auto& fa=get_anim(iid+1); smooth(fa.fill,*val?1.f:0.f,16.f);
    ImDrawList* dl=ImGui::GetWindowDrawList();
    float t=fa.fill;
    dl->AddRectFilled(tp,ImVec2(tp.x+tsz.x,tp.y+tsz.y),IM_COL32(44,44,58,255),11.f);
    if(t>0.01f) dl->AddRectFilled(tp,ImVec2(tp.x+tsz.x,tp.y+tsz.y),
        IM_COL32((int)(124+(255-124)*t),(int)(58+(255-58)*t),(int)(237+(255-237)*t), (int)(107+148*t)),11.f);
    float kx=tp.x+3+(tsz.x-6-18)*t;
    dl->AddCircleFilled(ImVec2(kx+9,tp.y+11),9.f,IM_COL32(240,240,245,255));
    if(t<0.5f) dl->AddCircle(ImVec2(kx+9,tp.y+11),9.f,IM_COL32(0,0,0,30));
    ImGui::SetCursorScreenPos(ImVec2(cp.x+4,cp.y+(rowH-ImGui::GetTextLineHeight())/2));
    ImGui::Text("%s",label);
    ImGui::SetCursorScreenPos(rowMax);
    ImGui::Dummy(ImVec2(0,0));
    return *val;
}

bool checkbox_row(const char* id, const char* label, bool* val, bool& blockDrag) {
    float rowH=layout::row_h; ImGuiID iid=ImGui::GetID(id);
    ImVec2 cp=ImGui::GetCursorScreenPos(), avail=ImGui::GetContentRegionAvail();
    ImVec2 rowMax(ImVec2(cp.x+avail.x,cp.y+rowH));
    auto& a=get_anim(iid); smooth(a.hover,ImGui::IsMouseHoveringRect(cp,rowMax)?1.f:0.f,14.f);
    if(a.hover>0.01f) {
        ImGui::GetWindowDrawList()->AddRectFilled(cp,rowMax,IM_COL32(255,255,255,(int)(a.hover*8.f)),4.f);
        blockDrag=true;
    }
    float cy=cp.y+rowH/2; ImVec2 cb(ImVec2(cp.x+avail.x-26,cy-9));
    ImGui::SetCursorScreenPos(cb);
    ImGui::InvisibleButton(id,ImVec2(18,18));
    if(ImGui::IsItemClicked()) *val=!*val;
    auto& fa=get_anim(iid+1); smooth(fa.fill,*val?1.f:0.f,16.f);
    ImDrawList* dl=ImGui::GetWindowDrawList(); float t=fa.fill;
    dl->AddRectFilled(cb,ImVec2(cb.x+18,cb.y+18),IM_COL32(44,44,58,255),4.f);
    if(t>0.01f) dl->AddRectFilled(cb,ImVec2(cb.x+18,cb.y+18),
        IM_COL32((int)(124+(255-124)*t),(int)(58+(255-58)*t),(int)(237+(255-237)*t),255),4.f);
    if(t>0.5f) { dl->AddLine(ImVec2(cb.x+4,cb.y+9),ImVec2(cb.x+7,cb.y+13),IM_COL32(255,255,255,255),2.f);
        dl->AddLine(ImVec2(cb.x+7,cb.y+13),ImVec2(cb.x+14,cb.y+4),IM_COL32(255,255,255,255),2.f); }
    ImGui::SetCursorScreenPos(ImVec2(cp.x+4,cp.y+(rowH-ImGui::GetTextLineHeight())/2));
    ImGui::Text("%s",label);
    ImGui::SetCursorScreenPos(rowMax);
    ImGui::Dummy(ImVec2(0,0));
    return *val;
}

void section_header(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text,colors::text_muted);
    ImGui::Text("%s",label); ImGui::PopStyleColor();
}

bool combo_row(const char* id, const char* label, int* idx, const char* items[], int cnt, bool& blockDrag) {
    float rowH=layout::row_h; ImVec2 cp=ImGui::GetCursorScreenPos(), avail=ImGui::GetContentRegionAvail();
    ImGui::SetCursorScreenPos(ImVec2(cp.x+4,cp.y+(rowH-ImGui::GetTextLineHeight())/2));
    ImGui::Text("%s",label);
    ImGui::SameLine(avail.x-148);
    ImGui::PushItemWidth(140); ImGui::SetCursorPosY(cp.y+(rowH-ImGui::GetFrameHeight())/2);
    bool changed=ImGui::Combo(id,idx,items,cnt); ImGui::PopItemWidth();
    if(ImGui::IsItemHovered()) blockDrag=true;
    ImGui::Dummy(ImVec2(0,0));
    return changed;
}

bool slider_float_row(const char* id, const char* label, float* val, float mn, float mx, const char* fmt, bool& blockDrag) {
    float rowH=layout::row_h; ImVec2 cp=ImGui::GetCursorScreenPos(), avail=ImGui::GetContentRegionAvail();
    ImGui::SetCursorScreenPos(ImVec2(cp.x+4,cp.y+(rowH-20)/2));
    ImGui::Text("%s",label);
    ImGui::SameLine(avail.x-152);
    ImGui::PushItemWidth(140); ImGui::SetCursorPosY(cp.y+(rowH-20)/2);
    bool changed=ImGui::DragFloat(id,val,0.5f,mn,mx,fmt,ImGuiSliderFlags_AlwaysClamp); ImGui::PopItemWidth();
    if(ImGui::IsItemHovered()||ImGui::IsItemActive()) blockDrag=true;
    ImGui::SetCursorPosY(cp.y+rowH);
    return changed;
}
