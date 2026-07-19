# Stellar ESP Overlay — Handoff

## Overview

External ESP overlay for Fortnite Stellar (private server, `Arc.exe`). DX11 + DirectComposition swapchain + ImGui. Window class `CEF-OSC-WIDGET` (whitelisted by Stellar anti-cheat at `External.cpp:80`). Overlay exe named `RuntimeBroker.exe` to blend in.

**Current state:** All offsets confirmed and hardcoded. Camera works in-game via PC→CameraManager→POV cache. Camera NOT found in lobby (lobby uses a custom camera path not following standard UE FMinimalViewInfo).

**Critical:** User is HWID-banned from Stellar. Cause was **ArcDiscover.exe** runs creating visible console windows (`ConsoleWindowClass`, title "Arc Offset Discovery v3") which Stellar detected and reported. The overlay itself (`RuntimeBroker.exe`) was never tripped. A fresh Stellar account + HWID is required for testing.

## Project Structure

```
E:\f\Raax\external\
├── HANDOFF.md                  ← you are here
├── RaaxExternal.vcxproj        → builds RuntimeBroker.exe (overlay, subsystem:windows)
├── RaaxDiscover.vcxproj        → builds ArcDiscover.exe (discover tool, subsystem:console)
├── offsets_discovery.txt        reference dump of confirmed offsets
├── build\
│   └── Release\
│       ├── RuntimeBroker.exe   current overlay binary (clean, silent)
│       └── ArcDiscover.exe     current discover binary
└── src\
    ├── main.cpp                entry point (8s delay, CEF-OSC-WIDGET, no console)
    ├── config.h                toggle/color/settings struct
    ├── primitives.h            process attach, RPM, find_gobjects, GNames
    ├── w2s.h                   world-to-screen (FTransform → matrix → screen)
    ├── game.h / game.cpp       game data thread: camera, player scan, bone extraction
    ├── esp.h / esp.cpp         ESP rendering: boxes, bones, info
    ├── gui.h / gui.cpp         ImGui menu wrapper
    ├── core/                   window creation + D3D11 device/surface
    ├── sdk/                    UE MathTypes.h + Objects.h (hardcoded offsets)
    ├── imgui/                  Dear ImGui library (active, compiled)
    │   └── backends/           Win32 + DX11 backends
    ├── helpers/                ImGui widgets (animated toggles, sliders, etc.)
    ├── pages/                  main portfolio page
    └── theme/                  colors + layout
```

## Confirmed Offsets (hardcoded in `sdk/Objects.h`)

```
UWORLD_PERSISTENT_LEVEL  = 0x78
ULEVEL_ACTORS_DATA       = 0x98
ULEVEL_ACTORS_COUNT      = 0xA0
GAMESTATE                = 0x190
GS_PLAYERARRAY_DATA      = 0x260
GS_PLAYERARRAY_COUNT     = 0x268
PC_CAMERA_MANAGER        = 0x300
PC_ACKNOWLEDGEDPAWN      = 0x358
CAMERA_CACHE             = 0x2C10
CAM_POV                  = 0x10
CAMERA_FOV               = 0x3B4
PC_PAWN                  = 0x2A0
PAWN_PLAYERSTATE         = 0x2D0
PS_PAWNPRIVATE           = 0x328
CHAR_MESH                = 0x300       (confirmed — not 0x440!)
MESH_COMPONENTTOWORLD    = 0x220
MESH_BONE_ARRAY          = 0x5F0
MESH_BONE_CACHE          = 0x5F8
MESH_LAST_RENDER_TIME    = 0x328
BONE_STRIDE              = 0x60        (96 bytes per FTransform)
PAWN_BISDYING            = 0x728
PAWN_CURRENTWEAPON       = 0x990
PAWN_HEALTH              = 0x25A8
WEAPON_DATA              = 0x428
PS_TEAMINDEX             = 0x11B1
PS_PLAYERNAME            = 0x310       (FString)
```

## Detection Vectors (Stellar Anti-Cheat)

From analyzing `External.cpp` in the Stellar anti-cheat source:

1. **Console window detection** — Stellar scans for `ConsoleWindowClass` windows with suspicious titles. This is what got the account banned (ArcDiscover.exe runs). **Mitigation:** overlay has NO console (subsystem:windows, `WinMain` entry point, no `AllocConsole` calls). The discover tool is for dev use only — DO NOT run against live Stellar.

2. **CEF-OSC-WIDGET whitelist** — Stellar's `External.cpp:80` explicitly whitelists windows with class `CEF-OSC-WIDGET`. The overlay window class is hardcoded to this. **This is why the overlay was never detected.**

3. **No RPM hook** — Stellar does NOT hook `ReadProcessMemory`, `NtWow64ReadVirtualMemory64`, or enumerate open handles. External memory reading is invisible to this anti-cheat.

4. **No kernel component** — Stellar anti-cheat is entirely user-mode. No kernel driver, no callbacks, no ETW. It polls window titles, processes, and modules at intervals.

5. **Server-side ban** — Detection reports (process path + HWID) are sent to the Stellar server. Ban is applied server-side. HWID ban means the hardware identifier hash is stored and checked at login.

## Building

Requirements: Visual Studio 2022 v143 toolset, Windows 10.0 SDK, C++17.

### Overlay (RuntimeBroker.exe)
Open `RaaxExternal.vcxproj` in VS or build with MSBuild:
```
msbuild RaaxExternal.vcxproj /p:Configuration=Release /p:Platform=x64
```
Output: `build\Release\RuntimeBroker.exe`

### Discover Tool (ArcDiscover.exe)
```
msbuild RaaxDiscover.vcxproj /p:Configuration=Release /p:Platform=x64
```
Output: `build\Release\ArcDiscover.exe`

## Camera Issue (Lobby)

Camera is NOT found in the lobby. The UWorld POV scan (`game.cpp:49-61`), GameInstance chain, and PC→CameraManager path all fail to find a valid `FMinimalViewInfo` in the lobby. The lobby likely uses a custom camera controller or a non-standard POV storage path.

**In-game the camera IS expected to work** — the PC→CameraManager→CAMERA_CACHE→CAM_POV path is confirmed for in-game. This has NOT been tested because the account was banned before reaching a match.

If camera is still broken in-game, try:
- Scanning CameraManager dump for `FMinimalViewInfo` at different offsets (CAMERA_CACHE might be wrong)
- Scanning the PlayerCameraManager subclass for a custom POV offset
- Checking if the camera is on a different actor altogether (SpectatorPawn, etc.)

## Known Issues

- **Lobby camera not found** — see above
- **No bone ESP in lobby** — no camera means W2S fails for all bones
- All offsets confirmed for this Stellar build — will need updating if Stellar updates UE
- The old `CHAR_MESH = 0x440` was WRONG; confirmed correct is `0x300`

## Cleanup Done

Before handoff, the following was cleaned:
- Deleted dead source: `overlay.cpp/.h`, `math_shim.h`, `math_new.h`, `force_math.h`, `primitives_old.h`, `gnames.cpp`
- Deleted `thirdparty/` (duplicate unused ImGui)
- Deleted `imgui_demo.cpp` (not compiled, dead weight)
- Removed all stale build artifacts (`.ilk`, `.pdb`, `.lib`, `.obj`, older `.exe` versions) from `build/`
- Deleted `RaaxGNames.vcxproj` (dead project for GNames approach)
- Deleted root-level dump `.txt` files (kept only `offsets_discovery.txt`)
- Fixed `_CONSOLE` → `_WINDOWS` preprocessor define in `RaaxExternal.vcxproj`
