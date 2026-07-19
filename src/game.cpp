#include "game.h"
#include "w2s.h"
#include <cmath>
#include <cstdio>
extern FILE* g_CrashLog;

static void ScanLog(const char* fmt, ...) {
    if (!g_CrashLog) return;
    char buf[256];
    va_list args; va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fputs(buf, g_CrashLog); fflush(g_CrashLog);
}

static void ReadPlayerName(uintptr_t ps, char* out, int max) {
    out[0]=0;
    int32_t len=Raax::read<int32_t>(ps+SDK::PS_PLAYERNAME+8);
    if(len<=0||len>64)return;
    uintptr_t data=Raax::read<uintptr_t>(ps+SDK::PS_PLAYERNAME);
    if(data<0x10000)return;
    wchar_t wbuf[128]={};
    int rlen=len<60?len:60;
    if(!Raax::rpm(data,wbuf,rlen*2))return;
    wbuf[rlen]=0;
    WideCharToMultiByte(CP_UTF8,0,wbuf,-1,out,max,0,0);
}

SDK::FVector GameData::GetBonePos(uintptr_t mesh, int boneId) {
    if(mesh<0x10000)return{};
    uintptr_t ba=Raax::read<uintptr_t>(mesh+SDK::MESH_BONE_ARRAY);
    if(ba<0x10000)ba=Raax::read<uintptr_t>(mesh+SDK::MESH_BONE_CACHE+0x10);
    if(ba<0x10000)return{};
    SDK::FTransform bl{}; if(!Raax::rpm(ba+boneId*SDK::BONE_STRIDE,&bl,sizeof(bl)))return{};
    SDK::FTransform ct{}; if(!Raax::rpm(mesh+SDK::MESH_COMPONENTTOWORLD,&ct,sizeof(ct)))return{};
    SDK::D3DMATRIX bm=SDK::TransformToMatrix(bl),wm=SDK::TransformToMatrix(ct),rm=SDK::MatrixMultiply(bm,wm);
    return{rm.m[3][0],rm.m[3][1],rm.m[3][2]};
}

static bool ValidateCamera(float* f) {
    if(!(f[0]>-500000&&f[0]<500000&&f[1]>-500000&&f[1]<500000&&f[2]>-500000&&f[2]<500000))return false;
    if(f[0]==0&&f[1]==0&&f[2]==0)return false;
    if(!(f[3]>-360&&f[3]<360&&f[4]>-360&&f[4]<360))return false;
    if(!(f[6]>40&&f[6]<200))return false;
    return true;
}

static DWORD WINAPI UpdateThreadProc(LPVOID) {
    GameData& gd = GameData::Get();
    static int once=0;
    while (gd.m_Running && Raax::alive()) {
        __try {
        SDK::UWorld world = SDK::FindWorld();
        if (!world.Valid) { Sleep(100); continue; }

        int actorCount = world.ActorCount();
        uintptr_t actorData = world.ActorData();
        if (!actorData) { Sleep(100); continue; }
        // Stale offset guard: a sane UE level has far fewer than 50k actors.
        // If the count looks insane, skip this frame instead of scanning garbage.
        if (actorCount < 1 || actorCount > 20000) { Sleep(100); continue; }


        // Camera: scan UWorld for pointer to FMinimalViewInfo
        gd.m_Camera.Valid=0;
        for(int off=0;off<=0x400&&!gd.m_Camera.Valid;off+=8){
            uintptr_t ptr=Raax::read<uintptr_t>(world.Addr+off);
            if(ptr<0x10000||ptr>0x7FFFFFFFFFFFULL)continue;
            float buf[8];
            if(!Raax::rpm(ptr,buf,32))continue;
            if(ValidateCamera(buf)){
                gd.m_Camera.Location={buf[0],buf[1],buf[2]};
                gd.m_Camera.Rotation={buf[3],buf[4],buf[5]};
                gd.m_Camera.FOV=buf[6];
                gd.m_Camera.Valid=1;
                if(!once){} // camera found
            }
        }

        // Find localPawn: scan actors for PlayerController with pawn
        uintptr_t localPawn=0;
        for(int i=0;i<actorCount&&i<50000&&!localPawn;i++){
            uintptr_t aa=Raax::read<uintptr_t>(actorData+i*8);if(aa<0x10000)continue;
            for(int off=0x280;off<=0x400&&!localPawn;off+=8){
                uintptr_t cm=Raax::read<uintptr_t>(aa+off);if(cm<0x10000)continue;
                for(int po=0x290;po<=0x370&&!localPawn;po+=8){
                    uintptr_t pn=Raax::read<uintptr_t>(aa+po);if(pn<0x10000)continue;
                    for(int mo=0x300;mo<=0x500&&!localPawn;mo+=8){
                        uintptr_t mh=Raax::read<uintptr_t>(pn+mo);if(mh>0x10000&&mh<0x7FFFFFFFFFFFULL){localPawn=pn;break;}
                    }
                }
            }
        }

        // If UWorld camera scan failed, try GameInstance path or PC→CameraManager
        if(!gd.m_Camera.Valid){
            // Scan actors for PlayerController with CameraManager
            for(int i=0;i<actorCount&&i<50000&&!gd.m_Camera.Valid;i++){
                uintptr_t aa=Raax::read<uintptr_t>(actorData+i*8);if(aa<0x10000)continue;
                for(int cmOff=0x280;cmOff<=0x380&&!gd.m_Camera.Valid;cmOff+=8){
                    uintptr_t cm=Raax::read<uintptr_t>(aa+cmOff);if(cm<0x10000)continue;
                    float buf[7];if(!Raax::rpm(cm+SDK::CAMERA_CACHE+SDK::CAM_POV,buf,28))continue;
                    if(ValidateCamera(buf)){
                        gd.m_Camera.Location={buf[0],buf[1],buf[2]};
                        gd.m_Camera.Rotation={buf[3],buf[4],buf[5]};
                        gd.m_Camera.FOV=buf[6];
                        gd.m_Camera.Valid=1;
                        if(!once){} // cam-pc found
                    }
                }
            }
        }

        // Collect players: scan actors for valid skeletal mesh
        PlayerCache players[MAX_PLAYERS];int pcnt=0;
        uintptr_t seen[128]={};int sn=0;
        int meshOffs[]={0x300,0x310,0x320,0x330,0x340,0x350,0x360,0x370,0x380,0x3E8,0x3F0,0x400,0x410,0x420,0x430,0x440,0x450,0x460,0x470,0x480,0x490,0x4A0,0x4B0,0x4C0,0x4D0,0x4E0,0x4F0,0x500};

        for(int i=0;i<actorCount&&i<50000&&pcnt<MAX_PLAYERS;i++){
            uintptr_t aa=Raax::read<uintptr_t>(actorData+i*8);if(aa<0x10000)continue;
            for(int mi=0;mi<27;mi++){
                uintptr_t mesh=Raax::read<uintptr_t>(aa+meshOffs[mi]);
                if(mesh<0x10000||mesh>0x7FFFFFFFFFFFULL||(mesh&7))continue;
                uintptr_t ba=Raax::read<uintptr_t>(mesh+SDK::MESH_BONE_ARRAY);
                if(ba<0x10000)ba=Raax::read<uintptr_t>(mesh+SDK::MESH_BONE_CACHE+0x10);
                if(ba<0x10000)continue;
                SDK::FVector root=gd.GetBonePos(mesh,0);
                if(root.X==0&&root.Y==0&&root.Z==0)continue;
                if(root.X!=root.X||root.Y!=root.Y||root.Z!=root.Z)continue;
                if(root.X>1e10f||root.X<-1e10f)continue;

                bool dup=false;for(int j=0;j<sn;j++)if(seen[j]==aa){dup=true;break;}
                if(dup)continue;if(sn<128)seen[sn++]=aa;
                if(!g_Config.ESP_ShowLocal&&aa==localPawn)continue;

                PlayerCache pl={};pl.Pawn=aa;pl.Mesh=mesh;pl.Visible=1;
                pl.Health=Raax::read<float>(aa+SDK::PAWN_HEALTH);if(pl.Health<1||pl.Health>200)pl.Health=0;
                uintptr_t ps=Raax::read<uintptr_t>(aa+SDK::PAWN_PLAYERSTATE);
                if(ps>0x10000){pl.TeamIndex=Raax::read<uint8_t>(ps+SDK::PS_TEAMINDEX);ReadPlayerName(ps,pl.Name,64);}
                pl.RootPos=root;if(gd.m_Camera.Valid)pl.DistanceM=gd.m_Camera.Location.Dist(pl.RootPos)/100.f;
                if(pl.DistanceM>g_Config.ESP_MaxDistance)continue;
                if(gd.m_Camera.Valid){
                    int bones[]={114,67,9,10,11,38,39,40,2,71,72,75,78,79,82,0};
                    for(int bi=0;bi<14;bi++){SDK::FVector bp=gd.GetBonePos(mesh,bones[bi]);Math::W2S(bp,gd.m_Camera.Location,gd.m_Camera.Rotation,gd.m_Camera.FOV,gd.m_ScreenW,gd.m_ScreenH,pl.BoneScreenPos[bi]);}
                    SDK::FVector hp=gd.GetBonePos(mesh,114);hp.Z+=15;
                    Math::W2S(hp,gd.m_Camera.Location,gd.m_Camera.Rotation,gd.m_Camera.FOV,gd.m_ScreenW,gd.m_ScreenH,pl.BoxTop);
                    Math::W2S(pl.RootPos,gd.m_Camera.Location,gd.m_Camera.Rotation,gd.m_Camera.FOV,gd.m_ScreenW,gd.m_ScreenH,pl.BoxBottom);
                    pl.OnScreen=(pl.BoxTop.X>0||pl.BoxBottom.X>0)?1:0;
                }
                players[pcnt++]=pl;
                goto nextActor;
            }
            nextActor:;
        }

        EnterCriticalSection(&gd.m_Lock);
        for(int i=0;i<pcnt;i++)gd.m_Players[i]=players[i];gd.m_PlayerCount=pcnt;
        LeaveCriticalSection(&gd.m_Lock);

        static int s_beat=0;
        if(++s_beat>=60){ s_beat=0; ScanLog("[SCAN] cam=%d players=%d actorCount=%d\n", gd.m_Camera.Valid?1:0, pcnt, actorCount); }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // Any access violation / bad read while scanning memory is swallowed.
            ScanLog("[SCAN] EXCEPTION 0x%X (cam reset)\n", GetExceptionCode());
            gd.m_Camera.Valid = 0;
            Sleep(100);
        }

        if(!once){once=1;}
        Sleep(5);
    }
    return 0;
}

void GameData::Update(){m_Running=1;HANDLE h=CreateThread(0,0,UpdateThreadProc,0,0,0);if(h)CloseHandle(h);}
