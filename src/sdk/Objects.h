#pragma once
#include "MathTypes.h"
#include "../primitives.h"
#include <Windows.h>

namespace SDK {

constexpr uint32_t UWORLD_PERSISTENT_LEVEL = 0x78;
constexpr uint32_t ULEVEL_ACTORS_DATA      = 0x98;
constexpr uint32_t ULEVEL_ACTORS_COUNT     = 0xA0;
constexpr uint32_t GAMESTATE               = 0x190;
constexpr uint32_t GS_PLAYERARRAY_DATA     = 0x260;  // TArray<PlayerState*>
constexpr uint32_t GS_PLAYERARRAY_COUNT    = 0x268;
constexpr uint32_t PC_CAMERA_MANAGER       = 0x300;
constexpr uint32_t PC_ACKNOWLEDGEDPAWN     = 0x358;
constexpr uint32_t CAMERA_CACHE            = 0x2C10;  // POV cache in CameraManager
constexpr uint32_t CAM_POV                 = 0x10;    // POV offset within cache entry
constexpr uint32_t CAMERA_FOV              = 0x3B4;
constexpr uint32_t PC_PAWN                 = 0x2A0;
constexpr uint32_t PAWN_PLAYERSTATE        = 0x2D0;
constexpr uint32_t PS_PAWNPRIVATE          = 0x328;
constexpr uint32_t CHAR_MESH               = 0x300;   // confirmed by discovery (Pawn+0x300)
constexpr uint32_t MESH_COMPONENTTOWORLD   = 0x220;
constexpr uint32_t MESH_BONE_ARRAY         = 0x5F0;
constexpr uint32_t MESH_BONE_CACHE         = 0x5F8;
constexpr uint32_t MESH_LAST_RENDER_TIME   = 0x328;
constexpr uint32_t BONE_STRIDE             = 0x60;   // 96 bytes per FTransform
constexpr uint32_t PAWN_BISDYING           = 0x728;
constexpr uint32_t PAWN_CURRENTWEAPON      = 0x990;
constexpr uint32_t PAWN_HEALTH             = 0x25A8;
constexpr uint32_t WEAPON_DATA             = 0x428;
constexpr uint32_t PS_TEAMINDEX            = 0x11B1;
constexpr uint32_t PS_PLAYERNAME           = 0x310;   // FString

struct UWorld {
    uintptr_t Addr; bool Valid;
    static UWorld Read(uintptr_t a) { return {a, a > 0x10000}; }
    uintptr_t GetLevel() const { return Raax::read<uintptr_t>(Addr + UWORLD_PERSISTENT_LEVEL); }
    int32_t   ActorCount() const { uintptr_t l = GetLevel(); if (l < 0x10000) return 0;
        return Raax::read<int32_t>(l + ULEVEL_ACTORS_COUNT); }
    uintptr_t ActorData() const { uintptr_t l = GetLevel(); if (l < 0x10000) return 0;
        return Raax::read<uintptr_t>(l + ULEVEL_ACTORS_DATA); }
    uintptr_t GetActor(int32_t i) const { uintptr_t d = ActorData(); if (!d || i < 0) return 0;
        return Raax::read<uintptr_t>(d + i * 8); }
};

struct Camera {
    FVector   Location;
    FRotator  Rotation;
    float     FOV;
    bool      Valid;

    static Camera Read() {
        Camera c{}; c.Valid = false;
        uintptr_t gobjs = Raax::find_gobjects();
        if (!gobjs) return c;
        int32_t n = Raax::read<int32_t>(gobjs + 0x14);

        // Find UWorld
        for (int32_t i = 0; i < n && i < 200000; i++) {
            uint32_t ci = i / 0x10000, oi = i % 0x10000;
            uintptr_t ch = Raax::read<uintptr_t>(gobjs + ci * 8); if (!ch) continue;
            uintptr_t p = Raax::read<uintptr_t>(ch + oi * 24); if (p < 0x10000) continue;
            uintptr_t lvl = Raax::read<uintptr_t>(p + UWORLD_PERSISTENT_LEVEL); if (lvl < 0x10000) continue;
            uintptr_t ad = Raax::read<uintptr_t>(lvl + ULEVEL_ACTORS_DATA);
            int32_t ac = Raax::read<int32_t>(lvl + ULEVEL_ACTORS_COUNT);
            if (ad < 0x10000 || ac < 50 || ac > 100000) continue;

            // Scan actors for PlayerController with valid camera
            for (int32_t ai = 0; ai < ac && ai < 5000; ai++) {
                uintptr_t aa = Raax::read<uintptr_t>(ad + ai * 8); if (aa < 0x10000) continue;
                uintptr_t cm = Raax::read<uintptr_t>(aa + PC_CAMERA_MANAGER); if (cm < 0x10000) continue;
                float x = Raax::read<float>(cm + CAMERA_CACHE + CAM_POV);
                float y = Raax::read<float>(cm + CAMERA_CACHE + CAM_POV + 4);
                float z = Raax::read<float>(cm + CAMERA_CACHE + CAM_POV + 8);
                if (x > -500000 && x < 500000 && y > -500000 && y < 500000 && z > -500000 && z < 500000 && !(x==0&&y==0&&z==0)) {
                    c.Location = {x, y, z};
                    c.Rotation = {
                        Raax::read<float>(cm + CAMERA_CACHE + CAM_POV + 0x24),
                        Raax::read<float>(cm + CAMERA_CACHE + CAM_POV + 0x28),
                        Raax::read<float>(cm + CAMERA_CACHE + CAM_POV + 0x2C)
                    };
                    // FOV scan
                    c.FOV = 90.f;
                    int fo_vals[] = {0x2B0,0x2B8,0x2C0,0x2C8,0x288,0x290,0x298,0x2A0};
                    for (int fi = 0; fi < 8; fi++) {
                        float f = Raax::read<float>(cm + fo_vals[fi]);
                        if (f >= 70.f && f <= 130.f) { c.FOV = f; break; }
                    }
                    c.Valid = true;
                    return c;
                }
            }
        }
        return c;
    }
};

inline UWorld FindWorld() {
    static uintptr_t cachedWorld = 0;
    if (cachedWorld > 0x10000) {
        uintptr_t lvl = Raax::read<uintptr_t>(cachedWorld + UWORLD_PERSISTENT_LEVEL);
        if (lvl > 0x10000 && Raax::safe_proc(lvl)) {
            int32_t ac = Raax::read<int32_t>(lvl + ULEVEL_ACTORS_COUNT);
            if (ac > 50 && ac < 100000) return {cachedWorld, true};
        }
        cachedWorld = 0;
    }
    uintptr_t gobjs = Raax::find_gobjects();
    if (!gobjs) return {};
    int32_t n = Raax::read<int32_t>(gobjs + 0x14);
    for (int32_t i = 0; i < n && i < 200000; i++) {
        uint32_t ci = i / 0x10000, oi = i % 0x10000;
        uintptr_t ch = Raax::read<uintptr_t>(gobjs + ci * 8); if (!ch) continue;
        uintptr_t p = Raax::read<uintptr_t>(ch + oi * 24); if (p < 0x10000) continue;
        uintptr_t lvl = Raax::read<uintptr_t>(p + UWORLD_PERSISTENT_LEVEL); if (lvl < 0x10000) continue;
        uintptr_t ad = Raax::read<uintptr_t>(lvl + ULEVEL_ACTORS_DATA);
        int32_t ac = Raax::read<int32_t>(lvl + ULEVEL_ACTORS_COUNT);
        if (ad > 0x10000 && ac > 50 && ac < 100000) { cachedWorld = p; return {p, true}; }
    }
    return {};
}

} // namespace SDK
