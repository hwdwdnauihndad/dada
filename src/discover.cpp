#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <stdint.h>
#include <cstdio>
#include <cmath>
#include <cstdlib>
static HANDLE g_Proc = nullptr;
static DWORD  g_Pid = 0;

int Attach(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do { if (!_wcsicmp(pe.szExeFile, name)) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    if (!pid) return 0;
    g_Proc = OpenProcess(PROCESS_VM_READ|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, pid);
    if (!g_Proc) return 0;
    g_Pid = pid;
    return 1;
}
void Detach() { if (g_Proc) { CloseHandle(g_Proc); g_Proc = nullptr; } }
int RPM(uintptr_t a, void* o, size_t s) { return ReadProcessMemory(g_Proc, (LPCVOID)a, o, s, nullptr); }
template<typename T> T Read(uintptr_t a) { T v{}; RPM(a, &v, sizeof(T)); return v; }

uintptr_t FindGObjects() {
    uint8_t pat[] = {0x48,0x8B,0x05,0,0,0,0,0x48,0x8B,0x0C,0xC8,0x48,0x8D,0x04,0xD1};
    size_t plen = 14;
    uintptr_t a = 0; MEMORY_BASIC_INFORMATION m;
    while (VirtualQueryEx(g_Proc, (LPCVOID)a, &m, sizeof(m))) {
        if (!(m.State == MEM_COMMIT && (m.Protect & (PAGE_READONLY|PAGE_READWRITE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE)))) {
            a = (uintptr_t)m.BaseAddress + m.RegionSize; continue;
        }
        if (m.RegionSize < plen) { a = (uintptr_t)m.BaseAddress + m.RegionSize; continue; }
        size_t sz = m.RegionSize; if (sz > 0x8000000) sz = 0x8000000;
        uint8_t* buf = (uint8_t*)HeapAlloc(GetProcessHeap(), 0, sz);
        if (!buf || !RPM((uintptr_t)m.BaseAddress, buf, sz)) { if (buf) HeapFree(GetProcessHeap(), 0, buf); a = (uintptr_t)m.BaseAddress + m.RegionSize; continue; }
        for (size_t i = 0; i + plen <= sz; i++) {
            int match = 1;
            for (size_t j = 0; j < plen; j++) if (pat[j] && buf[i+j] != pat[j]) { match = 0; break; }
            if (match) {
                int32_t rel;
                if (RPM((uintptr_t)m.BaseAddress + i + 3, &rel, 4)) { HeapFree(GetProcessHeap(), 0, buf); return (uintptr_t)m.BaseAddress + i + 7 + rel; }
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
        a = (uintptr_t)m.BaseAddress + m.RegionSize;
    }
    return 0;
}

// FMinimalViewInfo validation (7 consecutive floats)
static bool ValidCamFloats(float f[7]) {
    if(!(f[0]>-500000&&f[0]<500000&&f[1]>-500000&&f[1]<500000&&f[2]>-500000&&f[2]<500000))return false;
    if(f[0]==0&&f[1]==0&&f[2]==0)return false;
    if(!(f[3]>-360&&f[3]<360&&f[4]>-360&&f[4]<360))return false;
    if(!(f[6]>40&&f[6]<200))return false;
    return true;
}

int main() {
    printf("===========================================\n  Arc Offset Discovery v3\n=======================================\n\n");
    if(!Attach(L"Arc.exe")){printf("[-] Arc.exe not found.\n");return 1;}
    printf("[+] PID %u\n",g_Pid);

    uintptr_t gobjs=FindGObjects();
    if(!gobjs){printf("[-] GObjects not found.\n");Detach();return 1;}
    printf("[+] GObjects 0x%llX\n",gobjs);

    // Find UWorld
    int32_t n=Read<int32_t>(gobjs+0x14);
    uintptr_t world=0,actData=0;int actCount=0;
    for(int32_t i=0;i<n&&i<200000;i++){
        uint32_t ci=i/0x10000,oi=i%0x10000;
        uintptr_t ch=Read<uintptr_t>(gobjs+ci*8);if(!ch)continue;
        uintptr_t p=Read<uintptr_t>(ch+oi*24);if(p<0x10000)continue;
        uintptr_t lvl=Read<uintptr_t>(p+0x78);if(lvl<0x10000)continue;
        uintptr_t ad=Read<uintptr_t>(lvl+0x98);
        int32_t ac=Read<int32_t>(lvl+0xA0);
        if(ad>0x10000&&ac>10&&ac<200000){world=p;actData=ad;actCount=ac;break;}
    }
    if(!world){printf("[-] UWorld not found.\n");Detach();return 1;}
    printf("[+] UWorld 0x%llX  %d actors\n",world,actCount);

    FILE* out;
    fopen_s(&out,"E:\\f\\Raax\\offsets_discovery.txt","w");
    fprintf(out,"=== Arc Offset Discovery v3 ===\n");
    fprintf(out,"GObjects: 0x%llX\nUWorld: 0x%llX\nActors: %d\n\n",gobjs,world,actCount);

    printf("\n--- SCANNING UWorld for camera pointers ---\n");
    fprintf(out,"--- UWORLD CAMERA SCAN ---\n");

    // Scan UWorld for valid FMinimalViewInfo pointers (float-format) - wide range
    for(int off=0;off<=0x1000;off+=8){
        uintptr_t ptr=Read<uintptr_t>(world+off);
        if(ptr<0x10000)continue;
        float buf[7];
        if(!RPM(ptr,buf,28))continue;
        if(ValidCamFloats(buf)){
            printf("[CAM-FLOAT] UWorld+0x%X -> 0x%llX\n  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                off,ptr,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
            fprintf(out,"CAMERA FLOAT: UWorld+0x%X -> 0x%llX\n",off,ptr);
            fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=(%.1f,%.1f,%.1f)\n  FOV=%.1f\n\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
            goto foundCamUW;
        }
    }

    // Try double-format (UE5 FMinimalViewInfo uses doubles) - wide range
    for(int off=0;off<=0x1000;off+=8){
        uintptr_t ptr=Read<uintptr_t>(world+off);
        if(ptr<0x10000)continue;
        double buf[7];
        if(!RPM(ptr,buf,56))continue;
        if(!(buf[0]>-500000&&buf[0]<500000&&buf[1]>-500000&&buf[1]<500000&&buf[2]>-500000&&buf[2]<500000))continue;
        if(!(fabs(buf[0])>1000.0)||!(fabs(buf[1])>1000.0)||!(fabs(buf[2])>1000.0))continue;
        if(!(buf[3]>-360&&buf[3]<360&&buf[4]>-360&&buf[4]<360&&fabs(buf[3])>0.01))continue;
        printf("[CAM-DOUBLE] UWorld+0x%X -> 0x%llX\n  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f)\n",
            off,ptr,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
        fprintf(out,"CAMERA DOUBLE: UWorld+0x%X -> 0x%llX\n",off,ptr);
        fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=(%.1f,%.1f,%.1f)\n\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
        goto foundCamUW;
    }

    // Scan UWorld memory DIRECTLY for inline float camera data (7 consecutive floats)
    // This catches cases where FMinimalViewInfo is inline in UWorld (not pointer-to-struct)
    printf("  Scanning UWorld memory directly for inline camera data...\n");
    {
        uint8_t uwBuf[0x2000] = {};
        if(RPM(world, uwBuf, sizeof(uwBuf))){
            for(int off=0;off<(int)sizeof(uwBuf)-28;off+=4){
                float* f = (float*)(uwBuf+off);
                if(ValidCamFloats(f)){
                    printf("[INLINE-FLOAT] UWorld+0x%X  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                        off,f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
                    fprintf(out,"INLINE FLOAT: UWorld+0x%X\n",off);
                    fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=(%.1f,%.1f,%.1f)\n  FOV=%.1f\n\n",f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
                    goto foundCamUW;
                }
            }
        }
    }

    // Scan UWorld memory DIRECTLY for inline double camera data (7 consecutive doubles)
    {
        uint8_t uwBuf[0x2000] = {};
        if(RPM(world, uwBuf, sizeof(uwBuf))){
            for(int off=0;off<(int)sizeof(uwBuf)-56;off+=8){
                double* d = (double*)(uwBuf+off);
                if(d[0]>-500000&&d[0]<500000&&d[1]>-500000&&d[1]<500000&&d[2]>-500000&&d[2]<500000){
                    if(fabs(d[0])>1000.0||fabs(d[1])>1000.0||fabs(d[2])>1000.0){
                        if(d[3]>-360&&d[3]<360&&d[4]>-360&&d[4]<360&&fabs(d[3])>0.01){
                            printf("[INLINE-DOUBLE] UWorld+0x%X  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f)\n",
                                off,d[0],d[1],d[2],d[3],d[4],d[5]);
                            fprintf(out,"INLINE DOUBLE: UWorld+0x%X\n",off);
                            fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=(%.1f,%.1f,%.1f)\n\n",d[0],d[1],d[2],d[3],d[4],d[5]);
                            goto foundCamUW;
                        }
                    }
                }
            }
        }
    }

    // Try external cheat format: scan ALL pointer offsets for camera patterns
    printf("  Scanning all UWorld pointers for camera patterns...\n");
    for(int off=0;off<=0x1000;off+=8){
        uintptr_t ptr=Read<uintptr_t>(world+off);
        if(ptr<0x10000)continue;
        // Try float format at +0x00, +0x10, +0x20 within the target
        for(int delta=0;delta<=0x40;delta+=0x10){
            float buf[7]; if(!RPM(ptr+delta,buf,28))continue;
            if(ValidCamFloats(buf)){
                printf("[PTR-FLOAT@+0x%X] UWorld+0x%X -> 0x%llX+0x%X\n  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                    delta,off,ptr,delta,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                fprintf(out,"PTR FLOAT: UWorld+0x%X -> 0x%llX+0x%X\n",off,ptr,delta);
                fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=(%.1f,%.1f,%.1f)\n  FOV=%.1f\n\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                goto foundCamUW;
            }
        }
        // Try double format at +0x00, +0x10, +0x20 within the target
        for(int delta=0;delta<=0x40;delta+=0x10){
            double buf[7]; if(!RPM(ptr+delta,buf,56))continue;
            if(!(buf[0]>-500000&&buf[0]<500000&&buf[1]>-500000&&buf[1]<500000&&buf[2]>-500000&&buf[2]<500000))continue;
            if(!(fabs(buf[0])>1000.0)||!(fabs(buf[1])>1000.0)||!(fabs(buf[2])>1000.0))continue;
            if(!(buf[3]>-360&&buf[3]<360&&buf[4]>-360&&buf[4]<360))continue;
            printf("[PTR-DOUBLE@+0x%X] UWorld+0x%X -> 0x%llX+0x%X\n  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f)\n",
                delta,off,ptr,delta,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
            fprintf(out,"PTR DOUBLE: UWorld+0x%X -> 0x%llX+0x%X\n",off,ptr,delta);
            fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=(%.1f,%.1f,%.1f)\n\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
            goto foundCamUW;
        }
    }

    printf("[-] No UWorld camera found.\n");
    fprintf(out,"NO UWORLD CAMERA FOUND\n\n");

    foundCamUW:;

    // Also scan actors for CameraManager path (in-game)
    printf("\n--- Scanning actors for CameraManager ---\n");
    fprintf(out,"--- ACTOR CAMERA SCAN ---\n");
    uintptr_t foundPC=0,foundCM=0;int foundPCOff=0;
    for(int i=0;i<actCount&&i<80000&&!foundPC;i++){
        uintptr_t aa=Read<uintptr_t>(actData+i*8);if(aa<0x10000)continue;
        for(int off=0x280;off<=0x400&&!foundPC;off+=8){
            uintptr_t cm=Read<uintptr_t>(aa+off);if(cm<0x10000)continue;
            int povBases[]={0x2C10,0x2C00,0x2C20,0x2E00,0x2D00};
            for(int pb=0;pb<5&&!foundPC;pb++){
                float buf[7];
                if(!RPM(cm+povBases[pb]+0x10,buf,28))continue;
                if(ValidCamFloats(buf)){
                    foundPC=aa;foundCM=cm;foundPCOff=off;
                    printf("[CAM-ACTOR] Actor[%d]+0x%X CM=0x%llX POV@0x%X+0x10\n  Loc=(%.0f,%.0f,%.0f) FOV=%.1f\n",
                        i,off,cm,povBases[pb],buf[0],buf[1],buf[2],buf[6]);
                    fprintf(out,"CAMERA ACTOR: Actor+0x%X=CM, POV at CM+0x%X+0x10\n",off,povBases[pb]);
                    fprintf(out,"  Location=(%.0f,%.0f,%.0f)\n  Rotation=CM+POV+0x24/+0x28/+0x2C\n  FOV=%.1f\n\n",buf[0],buf[1],buf[2],buf[6]);
                    // Dump rotation
                    float pitch=Read<float>(cm+povBases[pb]+0x10+0x24);
                    float yaw=Read<float>(cm+povBases[pb]+0x10+0x28);
                    float roll=Read<float>(cm+povBases[pb]+0x10+0x2C);
                    fprintf(out,"  Pitch=%.1f Yaw=%.1f Roll=%.1f\n\n",pitch,yaw,roll);
                    printf("  Rot: P=%.1f Y=%.1f R=%.1f\n",pitch,yaw,roll);

                    // Find pawn
                    printf("[PAWN] Searching for pawn on PC...\n");
                    for(int po=0x280;po<=0x380;po+=8){
                        uintptr_t pn=Read<uintptr_t>(aa+po);if(pn<0x10000)continue;
                        for(int mo=0x300;mo<=0x500;mo+=8){
                            uintptr_t mh=Read<uintptr_t>(pn+mo);if(mh<0x10000)continue;
                            if(mh&7)continue;
                            uintptr_t ba=Read<uintptr_t>(mh+0x5F0);
                            if(ba<0x10000)ba=Read<uintptr_t>(mh+0x5F8+0x10);
                            if(ba<0x10000)continue;
                            printf("[PAWN] Found! Pawn@PC+0x%X=0x%llX Mesh@Pawn+0x%X=0x%llX BoneArray@0x%llX\n",po,pn,mo,mh,ba);
                            fprintf(out,"PAWN: PC+0x%X -> 0x%llX\n",po,pn);
                            fprintf(out,"MESH: Pawn+0x%X -> 0x%llX\n",mo,pn?mh:0);
                            fprintf(out,"BONE_ARRAY: Mesh+0x5F0 = 0x%llX\n",ba);
                            // ComponentToWorld
                            for(int co=0x1E0;co<=0x250;co+=8){
                                double t[3];
                                if(!RPM(mh+co,t,24))continue;
                                float fx=(float)t[0],fy=(float)t[1],fz=(float)t[2];
                                if(fx>-500000&&fx<500000&&fy>-500000&&fy<500000&&fz>-500000&&fz<500000&&!(fx==0&&fy==0&&fz==0)){
                                    printf("[CTW] Mesh+0x%X = (%.0f,%.0f,%.0f)\n",co,fx,fy,fz);
                                    fprintf(out,"COMPONENTTOWORLD: Mesh+0x%X\n\n",co);
                                }
                            }

                            // PlayerState
                            for(int pso=0x280;pso<=0x400;pso+=8){
                                uintptr_t ps=Read<uintptr_t>(pn+pso);if(ps<0x10000)continue;
                                for(int no=0x300;no<=0x400;no+=8){
                                    int32_t nl=Read<int32_t>(ps+no+8);
                                    uintptr_t nd=Read<uintptr_t>(ps+no);
                                    if(nl>0&&nl<64&&nd>0x10000){
                                        wchar_t wb[128]={};
                                        RPM(nd,wb,(nl<60?nl:60)*2);wb[nl]=0;
                                        if(wb[0]>=32&&wb[0]<127){
                                            printf("[PS] Pawn+0x%X=0x%llX Name@PS+0x%X='%ls'\n",pso,ps,no,wb);
                                            fprintf(out,"PLAYERSTATE: Pawn+0x%X\n",pso);
                                            fprintf(out,"PLAYERNAME: PS+0x%X\n",no);
                                            printf("[PS] Found name '%ls' at PS+0x%X\n",wb,no);
                                            goto donePS;
                                        }
                                    }
                                }
                            }
                            donePS:;

                            // bIsDying
                            for(int bo=0x700;bo<=0x800;bo++){
                                uint8_t v=Read<uint8_t>(pn+bo);
                                if((v&0x20)&&!(v>=32&&v<127)){
                                    printf("[DYE] Pawn+0x%X=0x%02X\n",bo,v);
                                    fprintf(out,"BISDYING: Pawn+0x%X\n\n",bo);
                                }
                            }

                            // Health
                            for(int ho=0x2300;ho<=0x2800;ho+=4){
                                float f=Read<float>(pn+ho);
                                if(f>=50.f&&f<=200.f){
                                    printf("[HP] Pawn+0x%X=%.1f\n",ho,f);
                                    fprintf(out,"HEALTH: Pawn+0x%X = %.1f\n\n",ho,f);
                                }
                            }

                            // Weapon
                            for(int wo=0x900;wo<=0xA00;wo+=8){
                                uintptr_t wp=Read<uintptr_t>(pn+wo);if(wp<0x10000)continue;
                                for(int wdo=0x400;wdo<=0x700;wdo+=8){
                                    uintptr_t wd=Read<uintptr_t>(wp+wdo);if(wd<0x10000)continue;
                                    for(int ino=0x40;ino<=0x80;ino+=8){
                                        int32_t nl=Read<int32_t>(wd+ino+8);
                                        uintptr_t nd=Read<uintptr_t>(wd+ino);
                                        if(nl>0&&nl<64&&nd>0x10000){
                                            wchar_t wb[128]={};RPM(nd,wb,(nl<60?nl:60)*2);wb[nl]=0;
                                            if(wb[0]>=32&&wb[0]<127){
                                                printf("[WP] Pawn+0x%X WD@W+0x%X ItemName='%ls' (+0x%X)\n",wo,wdo,wb,ino);
                                                fprintf(out,"WEAPON: Pawn+0x%X\n",wo);
                                                fprintf(out,"WEAPON_DATA: Weapon+0x%X\n",wdo);
                                                fprintf(out,"WEAPON_NAME: WD+0x%X\n\n",ino);
                                            }
                                        }
                                    }
                                }
                            }

                            goto pawnDone;
                        }
                    }
                    pawnDone:;
                }
            }
        }
    }
    if(!foundPC){
        printf("[-] No CameraManager on any actor (in-lobby mode)\n");
        fprintf(out,"NO ACTOR CAMERA FOUND (lobby mode only - UWorld camera above is lobby path)\n\n");
    }

    // GameState path (external cheat method)
    printf("\n--- GameState PlayerArray scan ---\n");
    fprintf(out,"--- GAMESTATE PLAYERS ---\n");
    int foundGSOff=0, foundPAOff=0;
    for(int gso=0x180;gso<=0x250;gso+=8){
        uintptr_t gs=Read<uintptr_t>(world+gso);if(gs<0x10000)continue;
        for(int pao=0x200;pao<=0x300;pao+=8){
            uintptr_t paData=Read<uintptr_t>(gs+pao);
            int32_t paCnt=Read<int32_t>(gs+pao+8);
            if(paData<0x10000||paCnt<=0||paCnt>256)continue;
            // Skip false positive: data ptr looks like ASCII (characters in the low byte)
            if((paData&0xFF)>=0x20&&(paData&0xFF)<0x80)continue; // filter ASCII-looking pointers
            if(((paData>>8)&0xFF)>=0x20&&((paData>>8)&0xFF)<0x80)continue;
            printf("[GS] UWorld+0x%X=GameState, PA@GS+0x%X cnt=%d\n",gso,pao,paCnt);
            fprintf(out,"GAMESTATE: UWorld+0x%X\n",gso);
            fprintf(out,"PLAYERARRAY: GS+0x%X (cnt=%d)\n\n",pao,paCnt);
            foundGSOff=gso; foundPAOff=pao;
            goto foundGS;
        }
    }
    // BROADER scan: try all TArray patterns across wider range
    if(!foundGSOff){
        printf("  Trying broader TArray scan on GameState...\n");
        uintptr_t gs=Read<uintptr_t>(world+0x190);
        if(gs>0x10000){
            for(int pao=0x200;pao<=0x380;pao+=8){
                uintptr_t paData=Read<uintptr_t>(gs+pao);
                int32_t paCnt=Read<int32_t>(gs+pao+8);
                if(paData>0x10000&&paCnt>0&&paCnt<256){
                    // Check if data ptr looks reasonable (NOT ASCII and NOT tiny)
                    if((paData&0xFFFF000000000000ULL)==0&&paData>0x10000){
                        printf("[GS-ALT] PA@GS+0x%X ptr=0x%llX cnt=%d\n",pao,paData,paCnt);
                        fprintf(out,"GAMESTATE_ALT: PA@GS+0x%X cnt=%d\n\n",pao,paCnt);
                    }
                }
            }
        }
    }
    foundGS:
    // Dump raw bytes around the PlayerArray data pointer for debugging
    {
        uintptr_t gs=Read<uintptr_t>(world+0x190);
        if(gs>0x10000){
            // Dump GS+0x240 to GS+0x270 to understand TArray layout
            printf("\n--- Dumping GS TArray region ---\n");
            fprintf(out,"\n--- GS TArray RAW DUMP ---\n");
            for(int po=0x240;po<=0x280;po+=8){
                uint64_t val=Read<uint64_t>(gs+po);
                int32_t cnt=Read<int32_t>(gs+po+8);
                int32_t max=Read<int32_t>(gs+po+0xC);
                printf("GS+0x%X: ptr=0x%llX cnt=%d max=%d\n",po,val,cnt,max);
                fprintf(out,"GS+0x%X: ptr=0x%llX cnt=%d max=%d\n",po,val,cnt,max);
                if(val>0x10000&&cnt>0&&cnt<256){
                    // Try reading first 3 entries from this array
                    for(int ei=0;ei<3&&ei<cnt;ei++){
                        uintptr_t e=Read<uintptr_t>(val+ei*8);
                        printf("  [%d] = 0x%llX\n",ei,e);
                        fprintf(out,"  [%d] = 0x%llX\n",ei,e);
                    }
                }
            }
        }
    }
    

    // If we found GameState PlayerArray, dump first 10 entries (including raw data even without pawns)
    {
        uintptr_t gs=Read<uintptr_t>(world+0x190);
        if(gs>0x10000){
            // Real TArray at GS+0x260 (based on dump: ptr=0x154863BA420 cnt=3)
            uintptr_t paData=Read<uintptr_t>(gs+0x260);
            int32_t paCnt=Read<int32_t>(gs+0x268);
            if(paData>0x10000&&paCnt>0&&paCnt<256){
                printf("\n--- PlayerArray dump (%d entries) ---\n",paCnt);
                fprintf(out,"\n--- PLAYERARRAY PAWN DUMP ---\n");
                for(int pi=0;pi<paCnt&&pi<10;pi++){
                    uintptr_t ps=Read<uintptr_t>(paData+pi*8);
                    if(ps<0x10000){printf("  [%d] null\n",pi);continue;}
                    // Try pawn at various offsets
                    uintptr_t pawn=0;
                    int pawnOffs[]={0x328,0x330,0x320,0x310,0x2F8,0x2F0,0x300,0x308,0x2E8,0x2E0,0x340,0x348,0x350};
                    for(int po=0;po<13&&!pawn;po++){
                        uintptr_t p=Read<uintptr_t>(ps+pawnOffs[po]);
                        if(p<0x10000)continue;
                        // Check if p has a mesh
                        for(int mo=0x300;mo<=0x500;mo+=8){
                            uintptr_t mesh=Read<uintptr_t>(p+mo);
                            if(mesh<0x10000||(mesh&7))continue;
                            uintptr_t ba=Read<uintptr_t>(mesh+0x5F0);
                            if(ba<0x10000)ba=Read<uintptr_t>(mesh+0x5F8+0x10);
                            if(ba>0x10000){pawn=p;goto foundPawn;}
                        }
                    }
                    foundPawn:;
                    if(pawn){
                        for(int mo=0x300;mo<=0x500;mo+=8){
                            uintptr_t mesh=Read<uintptr_t>(pawn+mo);
                            if(mesh<0x10000||(mesh&7))continue;
                            uintptr_t ba=Read<uintptr_t>(mesh+0x5F0);
                            if(ba<0x10000)ba=Read<uintptr_t>(mesh+0x5F8+0x10);
                            if(ba<0x10000)continue;
                            printf("[PL] PS[%d] Pawn@PS+0x%X=0x%llX Mesh@Pawn+0x%X=0x%llX BoneArray=0x%llX\n",pi,0,pawn,mo,mesh,ba);
                            fprintf(out,"PLAYER[%d]: PS+0x%X=Pawn, Pawn+0x%X=Mesh, Mesh+0x5F0=Bones\n",pi,0,mo);
                            // CTW
                            for(int co=0x1E0;co<=0x250;co+=8){
                                double t[3];if(!RPM(mesh+co,t,24))continue;
                                float fx=(float)t[0],fy=(float)t[1],fz=(float)t[2];
                                if(fx>-500000&&fx<500000&&fy>-500000&&fy<500000&&fz>-500000&&fz<500000&&!(fx==0&&fy==0&&fz==0)){
                                    fprintf(out,"  COMPONENTTOWORLD: Mesh+0x%X = (%.0f,%.0f,%.0f)\n",co,fx,fy,fz);
                                }
                            }
                            // Player name
                            for(int no=0x300;no<=0x400;no+=8){
                                int32_t nl=Read<int32_t>(ps+no+8);
                                uintptr_t nd=Read<uintptr_t>(ps+no);
                                if(nl>0&&nl<64&&nd>0x10000){
                                    wchar_t wb[128]={};RPM(nd,wb,(nl<60?nl:60)*2);wb[nl]=0;
                                    if(wb[0]>=32&&wb[0]<127)fprintf(out,"  NAME: PS+0x%X = '%ls'\n",no,wb);
                                }
                            }
                            fprintf(out,"\n");
                            break;
                        }
                    } else {
                        // No pawn found - dump PS info anyway
                        printf("  PS[%d] = 0x%llX (no pawn)\n",pi,ps);
                        fprintf(out,"PLAYER[%d]: PS=0x%llX (no pawn)\n",pi,ps);
                        // Scan PS for name
                        for(int no=0x300;no<=0x400;no+=8){
                            int32_t nl=Read<int32_t>(ps+no+8);
                            uintptr_t nd=Read<uintptr_t>(ps+no);
                            if(nl>0&&nl<64&&nd>0x10000){
                                wchar_t wb[128]={};RPM(nd,wb,(nl<60?nl:60)*2);wb[nl]=0;
                                if(wb[0]>=32&&wb[0]<127){
                                    printf("  Name@PS+0x%X = '%ls'\n",no,wb);
                                    fprintf(out,"  NAME: PS+0x%X = '%ls'\n\n",no,wb);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Scan ALL actors for ANY camera-like data
    printf("\n--- Scanning all actors for camera-like data ---\n");
    fprintf(out,"\n--- GLOBAL ACTOR CAMERA SCAN ---\n");
    {
        int camFound=0;
        for(int i=0;i<actCount&&i<2000&&!camFound;i++){
            uintptr_t aa=Read<uintptr_t>(actData+i*8);if(aa<0x10000)continue;
            // Try CameraManager at various offsets on each actor
            for(int cmOff=0x280;cmOff<=0x380&&!camFound;cmOff+=8){
                uintptr_t cm=Read<uintptr_t>(aa+cmOff);if(cm<0x10000)continue;
                // Try many POV offsets within the CameraManager
                int povTests[]={0x2C10,0x2C00,0x2C20,0x2E00,0x2D00,0x2B00,0x2B10,0x2B20,0x2A00,0x1C10,0x1C00,0x1C20};
                for(int pt=0;pt<12&&!camFound;pt++){
                    for(int delta=0;delta<=0x30;delta+=0x10){
                        float buf[7];if(!RPM(cm+povTests[pt]+delta,buf,28))continue;
                        if(ValidCamFloats(buf)){
                            printf("[CAM-ALL] Actor[%d]+0x%X=CM, CM+0x%X+0x%X=POV\n  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                                i,cmOff,povTests[pt],delta,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                            fprintf(out,"CAMERA: Actor+0x%X=CM, CM+0x%X+0x%X=POV\n",cmOff,povTests[pt],delta);
                            fprintf(out,"  Location=(%.0f,%.0f,%.0f) Rotation=(%.1f,%.1f,%.1f) FOV=%.1f\n\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                            camFound=1;
                        }
                    }
                }
            }
        }
        if(!camFound){
            // Scan actors WITHOUT assuming CameraManager format - try every 8-byte field
            printf("  Trying raw pointer scan on first 1000 actors...\n");
            for(int i=0;i<actCount&&i<1000&&!camFound;i++){
                uintptr_t aa=Read<uintptr_t>(actData+i*8);if(aa<0x10000)continue;
                for(int ao=0x280;ao<=0x400&&!camFound;ao+=8){
                    uintptr_t ptr=Read<uintptr_t>(aa+ao);
                    if(ptr<0x10000||ptr>0x7FFFFFFFFFFFULL)continue;
                    for(int delta=0;delta<=0x100;delta+=0x10){
                        float buf[7];if(!RPM(ptr+delta,buf,28))continue;
                        if(ValidCamFloats(buf)){
                            printf("[CAM-RAW] Actor[%d]+0x%X -> 0x%llX+0x%X\n  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                                i,ao,ptr,delta,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                            fprintf(out,"CAMERA RAW: Actor+0x%X -> 0x%llX+0x%X\n  Loc Rot FOV\n\n",ao,ptr,delta);
                            camFound=1;
                        }
                    }
                }
            }
        }
        if(!camFound){
            printf("[-] No camera data on any actor.\n");
            fprintf(out,"NO CAMERA DATA ON ANY ACTOR\n\n");
        }
    }

    // NEW: Follow UWorld -> GameInstance -> LocalPlayer -> PlayerController -> Camera
    printf("\n--- UWorld -> GameInstance/LocalPlayer/PC chain ---\n");
    fprintf(out,"\n--- GAMEINSTANCE CHAIN ---\n");
    int giCamFound=0;
    // First scan ALL UWorld pointer fields for potential GameInstance (wide range)
    for(int giOff=0x198;giOff<=0x1E0;giOff+=8){
        uintptr_t gi=Read<uintptr_t>(world+giOff);
        if(gi<0x10000)continue;
        // Check if it looks like a GameInstance (has LocalPlayers array)
        for(int lpOff=0x38;lpOff<=0x80;lpOff+=8){
            uintptr_t lpArr=Read<uintptr_t>(gi+lpOff);
            int32_t lpCnt=Read<int32_t>(gi+lpOff+8);
            if(lpArr>0x10000&&lpCnt>0&&lpCnt<10){
                printf("[GI] UWorld+0x%X=GameInstance=0x%llX, LParr@GI+0x%X, cnt=%d\n",giOff,gi,lpOff,lpCnt);
                fprintf(out,"GAMEINSTANCE: UWorld+0x%X\n",giOff);
                fprintf(out,"LOCALPLAYER_ARR: GI+0x%X (cnt=%d)\n",lpOff,lpCnt);
                // Read first LocalPlayer
                uintptr_t lp=Read<uintptr_t>(lpArr);
                if(lp>0x10000){
                    fprintf(out,"LOCALPLAYER: 0x%llX\n",lp);
                    printf("  LocalPlayer=0x%llX\n",lp);
                    // Try to find PlayerController
                    for(int pcOff=0x30;pcOff<=0x80;pcOff+=8){
                        uintptr_t pc=Read<uintptr_t>(lp+pcOff);
                        if(pc>0x10000){
                            printf("  PlayerController@LP+0x%X=0x%llX\n",pcOff,pc);
                            fprintf(out,"PLAYERCONTROLLER: LP+0x%X\n",pcOff);
                            // Try CameraManager on this PC
                            for(int cmOff=0x280;cmOff<=0x400;cmOff+=8){
                                uintptr_t cm=Read<uintptr_t>(pc+cmOff);
                                if(cm<0x10000)continue;
                                int povTests[]={0x2C10,0x2C00,0x2C20,0x2E00,0x2D00,0x2B00,0x2B10,0x1C10,0x1C00,0x1000};
                                for(int pt=0;pt<10;pt++){
                                    float buf[7];if(!RPM(cm+povTests[pt]+0x10,buf,28))continue;
                                    if(ValidCamFloats(buf)){
                                        printf("  [CAM-PC] PC+0x%X=CM=0x%llX CM+0x%X+0x10=POV\n    Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                                            cmOff,cm,povTests[pt],buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                                        fprintf(out,"CAMERA_PC: PC+0x%X=CM, CM+0x%X+0x10=POV\n",cmOff,povTests[pt]);
                                        // Dump rotation
                                        float pitch=Read<float>(cm+povTests[pt]+0x10+0x24);
                                        float yaw=Read<float>(cm+povTests[pt]+0x10+0x28);
                                        float roll=Read<float>(cm+povTests[pt]+0x10+0x2C);
                                        fprintf(out,"  Pitch=%.1f Yaw=%.1f Roll=%.1f\n\n",pitch,yaw,roll);
                                        giCamFound=1;
                                        goto foundGI;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    foundGI:;
    if(!giCamFound) printf("[-] No camera via GameInstance chain.\n");

    // NEW: Scan UWorld memory up to 16KB for any FOV-like float (trust the FOV)
    printf("\n--- Scanning UWorld memory for FOV float pattern ---\n");
    fprintf(out,"\n--- UWORLD FOV SCAN ---\n");
    {
        uint8_t uwBig[0x4000]={};
        if(RPM(world,uwBig,sizeof(uwBig))){
            int fovFound=0;
            for(int off=0;off<(int)sizeof(uwBig)-28;off+=4){
                float* f=(float*)(uwBig+off);
                if(f[6]>=70.f&&f[6]<=130.f){
                    // Found candidate FOV - check if location and rotation are valid
                    if(!(f[0]>-500000&&f[0]<500000&&f[1]>-500000&&f[1]<500000&&f[2]>-500000&&f[2]<500000))continue;
                    if(f[0]==0&&f[1]==0&&f[2]==0)continue;
                    if(!(f[3]>-360&&f[3]<360&&f[4]>-360&&f[4]<360))continue;
                    printf("  [FOV-CAM] UWorld+0x%X  Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                        off,f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
                    fprintf(out,"FOV CAM: UWorld+0x%X\n  Location=(%.0f,%.0f,%.0f) Rotation=(%.1f,%.1f,%.1f) FOV=%.1f\n\n",
                        off,f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
                    fovFound=1;
                    if(!giCamFound) giCamFound=1;
                    if(fovFound>=3)break; // capture at most 3 candidates
                }
            }
            if(!fovFound) printf("  No FOV pattern in UWorld memory.\n");
        }
    }

    // Try finding camera through the confirmed player pawn's owner chain
    printf("\n--- Trying to find camera via pawn owner chain ---\n");
    fprintf(out,"\n--- PAWN OWNER CHAIN ---\n");
    {
        uintptr_t knownPawn=0x15489C6BDC0; // confirmed from previous run
        // Verify pawn still valid
        uintptr_t mesh=Read<uintptr_t>(knownPawn+0x300);
        if(mesh>0x10000&&(mesh&7)==0){
            uintptr_t ba=Read<uintptr_t>(mesh+0x5F0);
            if(ba>0x10000){
                printf("  Verified pawn=0x%llX mesh=0x%llX boneArray=0x%llX\n",knownPawn,mesh,ba);
                fprintf(out,"PAWN: 0x%llX\nMESH: +0x300 = 0x%llX\nBONES: +0x5F0 = 0x%llX\n\n",knownPawn,mesh,ba);
                
                // Find Owner (scan pawn for pointer to PC)
                uintptr_t owner=0;
                int ownerOffs[]={0x18,0x20,0x28,0x30};
                for(int oi=0;oi<4&&!owner;oi++){
                    uintptr_t o=Read<uintptr_t>(knownPawn+ownerOffs[oi]);
                    if(o>0x10000){
                        // Check if o has CameraManager field
                        for(int cmOff=0x280;cmOff<=0x400;cmOff+=8){
                            uintptr_t cm=Read<uintptr_t>(o+cmOff);
                            if(cm<0x10000)continue;
                            // Try POV offsets
                            int povTests[]={0x2C10,0x2C00,0x2C20,0x2E00,0x2D00};
                            for(int pt=0;pt<5;pt++){
                                float buf[7];if(!RPM(cm+povTests[pt]+0x10,buf,28))continue;
                                if(ValidCamFloats(buf)){
                                    printf("  [CAM-OWNER] Pawn+0x%X=Owner=0x%llX, CM@Owner+0x%X, POV@CM+0x%X+0x10\n    Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                                        ownerOffs[oi],o,cmOff,povTests[pt],buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                                    fprintf(out,"CAMERA: Pawn+0x%X=Owner, Owner+0x%X=CM, CM+0x%X+0x10=POV\n",ownerOffs[oi],cmOff,povTests[pt]);
                                    giCamFound=1; owner=o; goto foundCamOwner;
                                }
                            }
                        }
                    }
                }
                foundCamOwner:;
                if(!owner){
                    printf("  No owner found on pawn.\n");
                    // Try scanning ALL actors for one that acknowledges this pawn
                    printf("  Scanning %d actors for one referencing this pawn...\n",actCount);
                    for(int i=0;i<actCount&&i<80000;i++){
                        uintptr_t aa=Read<uintptr_t>(actData+i*8);
                        if(aa<0x10000||aa==knownPawn)continue;
                        // Check if this actor has a pointer to our pawn (PawnPrivate or AcknowledgedPawn)
                        for(int po=0x290;po<=0x380;po+=8){
                            uintptr_t pv=Read<uintptr_t>(aa+po);
                            if(pv==knownPawn){
                                printf("  [PC] Actor[%d]+0x%X points to pawn -> this could be PlayerController\n",i,po);
                                fprintf(out,"PC: Actor[%d]+0x%X -> 0x%llX (references our pawn)\n",i,po,aa);
                                // Check if this actor has CameraManager
                                for(int cmOff=0x280;cmOff<=0x400;cmOff+=8){
                                    uintptr_t cm=Read<uintptr_t>(aa+cmOff);
                                    if(cm<0x10000)continue;
                                    int povTests[]={0x2C10,0x2C00,0x2C20,0x2E00,0x2D00};
                                    for(int pt=0;pt<5;pt++){
                                        float buf[7];if(!RPM(cm+povTests[pt]+0x10,buf,28))continue;
                                        if(ValidCamFloats(buf)){
                                            printf("  [CAM-PC2] Actor[%d]+0x%X=CM, CM+0x%X+0x10=POV\n    Loc=(%.0f,%.0f,%.0f) Rot=(%.1f,%.1f,%.1f) FOV=%.1f\n",
                                                i,cmOff,povTests[pt],buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
                                            fprintf(out,"CAMERA2: Actor+0x%X=CM, CM+0x%X+0x10=POV\n",cmOff,povTests[pt]);
                                            giCamFound=1; goto foundCamOwner2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    foundCamOwner2:;
                    if(!giCamFound) printf("  No referencing actor found.\n");
                }
            }
        } else {
            printf("  Known pawn no longer valid (game may have changed).\n");
        }
    }

    fprintf(out,"\n=== SUMMARY ===\n");
    fprintf(out,"GObjects: pattern scan\n");
    fprintf(out,"UWorld.PersistentLevel: +0x78\n");
    fprintf(out,"ULevel.Actors: +0x98(data) +0xA0(count)\n");
    fclose(out);

    printf("\n[+] Done -> offsets_discovery.txt\n");
    printf("    Paste the contents so offsets can be hardcoded.\n");
    Detach();
    system("pause");
    return 0;
}