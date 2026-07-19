#pragma once

struct Config {
    bool ESP_Enabled = false;
    bool ESP_Boxes = false;
    bool ESP_BoxCornerMode = false;
    float ESP_BoxThickness = 1.5f;
    bool ESP_Skeleton = false;
    float ESP_SkeletonThickness = 1.f;
    bool ESP_Snaplines = false;
    int ESP_SnaplineOrigin = 0;
    bool ESP_Names = true;
    bool ESP_Distance = true;
    bool ESP_Health = true;
    bool ESP_Weapon = true;
    bool ESP_HeadDot = false;
    bool ESP_TeamCheck = true;
    bool ESP_ShowLocal = true;
    float ESP_MaxDistance = 500.f;
    float ESP_BoxColor[4] = {1,0,0,1};
    float ESP_SkeletonColor[4] = {1,1,1,1};
    float ESP_SnaplineColor[4] = {1,1,0,0.8f};
    float ESP_NameColor[4] = {1,1,1,1};
    float ESP_HealthHigh[4] = {0,1,0,1};
    float ESP_HealthLow[4] = {1,0,0,1};
};

inline Config g_Config;
