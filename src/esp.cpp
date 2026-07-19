#include "esp.h"
#include "game.h"
#include "config.h"
#include <cmath>
#include <cstdio>

static void DrawCornerBox(ImDrawList* dl, float x1, float y1, float x2, float y2, ImU32 col, float t) {
    float dx=x2>x1?x2-x1:x1-x2, dy=y2>y1?y2-y1:y1-y2;
    float len=(dx<dy?dx:dy)*0.20f;
    if(len<4.f) len=4.f; if(len>22.f) len=22.f;
    ImU32 shadow=IM_COL32(0,0,0,200);
    float ts=t+1.5f;
    dl->AddLine(ImVec2(x1,y1),ImVec2(x1+len,y1),shadow,ts); dl->AddLine(ImVec2(x1,y1),ImVec2(x1,y1+len),shadow,ts);
    dl->AddLine(ImVec2(x2,y1),ImVec2(x2-len,y1),shadow,ts); dl->AddLine(ImVec2(x2,y1),ImVec2(x2,y1+len),shadow,ts);
    dl->AddLine(ImVec2(x1,y2),ImVec2(x1+len,y2),shadow,ts); dl->AddLine(ImVec2(x1,y2),ImVec2(x1,y2-len),shadow,ts);
    dl->AddLine(ImVec2(x2,y2),ImVec2(x2-len,y2),shadow,ts); dl->AddLine(ImVec2(x2,y2),ImVec2(x2,y2-len),shadow,ts);
    dl->AddLine(ImVec2(x1,y1),ImVec2(x1+len,y1),col,t); dl->AddLine(ImVec2(x1,y1),ImVec2(x1,y1+len),col,t);
    dl->AddLine(ImVec2(x2,y1),ImVec2(x2-len,y1),col,t); dl->AddLine(ImVec2(x2,y1),ImVec2(x2,y1+len),col,t);
    dl->AddLine(ImVec2(x1,y2),ImVec2(x1+len,y2),col,t); dl->AddLine(ImVec2(x1,y2),ImVec2(x1,y2-len),col,t);
    dl->AddLine(ImVec2(x2,y2),ImVec2(x2-len,y2),col,t); dl->AddLine(ImVec2(x2,y2),ImVec2(x2,y2-len),col,t);
}

void ESP::Render(ImDrawList* dl, int sw, int sh) {
    GameData& gd=GameData::Get();

    dl->AddLine(ImVec2((float)sw/2-20,(float)sh/2),ImVec2((float)sw/2+20,(float)sh/2),IM_COL32(255,0,0,255),2.f);
    dl->AddLine(ImVec2((float)sw/2,(float)sh/2-20),ImVec2((float)sw/2,(float)sh/2+20),IM_COL32(255,0,0,255),2.f);
    char dbg[64]; _snprintf(dbg,64,"%dx%d cam:%d pl:%d",sw,sh,gd.m_Camera.Valid?1:0,gd.m_PlayerCount);
    dl->AddText(ImVec2(10,10),IM_COL32(0,255,0,255),dbg);

    EnterCriticalSection(&gd.m_Lock);

    for(int pi=0;pi<gd.m_PlayerCount;pi++) {
        PlayerCache& p=gd.m_Players[pi];
        if(!p.OnScreen) continue;

        float x=p.BoxBottom.X, y=p.BoxBottom.Y;
        float h=p.BoxTop.Y-p.BoxBottom.Y; if(h<0)h=-h;
        float w=h*0.5f;
        float x1=x-w/2,x2=x+w/2,y1=p.BoxTop.Y,y2=p.BoxBottom.Y;

        ImU32 bc=IM_COL32((int)(g_Config.ESP_BoxColor[0]*255),(int)(g_Config.ESP_BoxColor[1]*255),(int)(g_Config.ESP_BoxColor[2]*255),(int)(g_Config.ESP_BoxColor[3]*255));
        ImU32 sc=IM_COL32((int)(g_Config.ESP_SkeletonColor[0]*255),(int)(g_Config.ESP_SkeletonColor[1]*255),(int)(g_Config.ESP_SkeletonColor[2]*255),(int)(g_Config.ESP_SkeletonColor[3]*255));
        ImU32 slc=IM_COL32((int)(g_Config.ESP_SnaplineColor[0]*255),(int)(g_Config.ESP_SnaplineColor[1]*255),(int)(g_Config.ESP_SnaplineColor[2]*255),(int)(g_Config.ESP_SnaplineColor[3]*255));
        ImU32 nc=IM_COL32((int)(g_Config.ESP_NameColor[0]*255),(int)(g_Config.ESP_NameColor[1]*255),(int)(g_Config.ESP_NameColor[2]*255),(int)(g_Config.ESP_NameColor[3]*255));

        if(g_Config.ESP_Boxes){
            float t=g_Config.ESP_BoxThickness;
            ImU32 shadow=IM_COL32(0,0,0,200);
            if(g_Config.ESP_BoxCornerMode){
                DrawCornerBox(dl,x1,y1,x2,y2,bc,t);
            } else {
                dl->AddRect(ImVec2(x1-1,y1-1),ImVec2(x2+1,y2+1),shadow,0.f,0,t+1.5f);
                dl->AddRect(ImVec2(x1,y1),ImVec2(x2,y2),bc,0.f,0,t);
            }
        }

        if(g_Config.ESP_Health){
            float health=p.Health;
            if(health<0)health=0; if(health>200)health=100;
            float barX=x1-7, barH=y2-y1, barW=3;
            float fillH=barH*(fminf(health,100.f)/100.f);
            dl->AddRectFilled(ImVec2(barX-1,y1-1),ImVec2(barX+barW+1,y2+1),IM_COL32(0,0,0,150));
            ImU32 hi=IM_COL32(0,220,80,255), lo=IM_COL32(220,40,40,255);
            dl->AddRectFilled(ImVec2(barX,y2-fillH),ImVec2(barX+barW,y2),lo);
            dl->AddRectFilled(ImVec2(barX,y2-fillH),ImVec2(barX+barW,y2-fillH*0.5f),hi);
        }

        if(g_Config.ESP_Skeleton){
            struct{int a,b;} bones[]={
                {1,2},{2,3},{3,4},{1,5},{5,6},{6,7},{1,8},{8,9},{9,10},{10,11},{8,12},{12,13},{13,14},
            };
            float st=g_Config.ESP_SkeletonThickness;
            for(auto& b:bones){
                auto& ba=p.BoneScreenPos[b.a],&bb=p.BoneScreenPos[b.b];
                if(ba.X>0&&ba.Y>0&&bb.X>0&&bb.Y>0) {
                    dl->AddLine(ImVec2(ba.X,ba.Y),ImVec2(bb.X,bb.Y),IM_COL32(0,0,0,200),st+1.5f);
                    dl->AddLine(ImVec2(ba.X,ba.Y),ImVec2(bb.X,bb.Y),sc,st);
                }
            }
        }

        if(g_Config.ESP_HeadDot){
            auto& hd=p.BoneScreenPos[0];
            if(hd.X>0&&hd.Y>0){
                dl->AddCircleFilled(ImVec2(hd.X,hd.Y),4.5f,IM_COL32(0,0,0,180));
                dl->AddCircleFilled(ImVec2(hd.X,hd.Y),3.5f,IM_COL32(255,255,255,255));
            }
        }

        if(g_Config.ESP_Snaplines){
            float originX=(float)sw/2;
            float originY=g_Config.ESP_SnaplineOrigin==1?(float)sh/2:(float)sh;
            dl->AddLine(ImVec2(originX,originY),ImVec2(x,y),IM_COL32(0,0,0,200),2.5f);
            dl->AddLine(ImVec2(originX,originY),ImVec2(x,y),slc,1.5f);
        }

        if(g_Config.ESP_Names||g_Config.ESP_Distance||g_Config.ESP_Weapon){
            char buf[256]={}; int off=0;
            if(g_Config.ESP_Names&&p.Name[0]) off+=_snprintf(buf+off,256-off,"%s",p.Name);
            if(g_Config.ESP_Weapon&&p.Weapon[0]) off+=_snprintf(buf+off,256-off,"%s[%s]",(off>0?" " :""),p.Weapon);
            if(g_Config.ESP_Distance) off+=_snprintf(buf+off,256-off,"%s%.0fm",(off>0?" ":""),p.DistanceM);
            if(off>0){
                dl->AddText(ImVec2(x1+2,y1-18),IM_COL32(0,0,0,180),buf);
                dl->AddText(ImVec2(x1+1,y1-19),nc,buf);
            }
        }
    }
    LeaveCriticalSection(&gd.m_Lock);
}
