#pragma once
#include "sdk/Objects.h"
#include "primitives.h"
#include "config.h"
#include <Windows.h>

#define MAX_PLAYERS 128
#define MAX_BONES 16

struct PlayerCache {
    uintptr_t Pawn, Mesh;
    int TeamIndex;
    int IsDBNO;
    float DistanceM;
    SDK::FVector RootPos;
    SDK::FVector2D BoneScreenPos[MAX_BONES];
    SDK::FVector2D BoxTop, BoxBottom;
    int OnScreen;
    char Name[64];
    char Weapon[64];
    float Health;
    int Visible;
};

class GameData {
  public:
    SDK::Camera m_Camera;
    PlayerCache m_Players[MAX_PLAYERS];
    int m_PlayerCount;
    CRITICAL_SECTION m_Lock;
    int m_ScreenW, m_ScreenH;
    volatile LONG m_Running;

    static GameData& Get() { static GameData g; return g; }
    GameData() : m_PlayerCount(0), m_ScreenW(1920), m_ScreenH(1080), m_Running(0) { InitializeCriticalSection(&m_Lock); }
    ~GameData() { DeleteCriticalSection(&m_Lock); }

    void Update();
    SDK::FVector GetBonePos(uintptr_t mesh, int boneId);
    void ReadStringW(uintptr_t addr, wchar_t* out, int max);
};
