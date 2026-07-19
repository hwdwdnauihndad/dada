#pragma once
#include "sdk/MathTypes.h"
namespace Math {
constexpr float PI=3.14159265358979f,D2R=PI/180.f,R2D=180.f/PI;
inline SDK::D3DMATRIX RotationMatrix(const SDK::FRotator& r){
    float sp=(float)sin(r.Pitch*D2R),cp=(float)cos(r.Pitch*D2R),sy=(float)sin(r.Yaw*D2R),cy=(float)cos(r.Yaw*D2R),sr=(float)sin(r.Roll*D2R),cr=(float)cos(r.Roll*D2R);
    SDK::D3DMATRIX m{};m.m[0][0]=cp*cy;m.m[0][1]=cp*sy;m.m[0][2]=sp;m.m[0][3]=0.f;
    m.m[1][0]=sr*sp*cy-cr*sy;m.m[1][1]=sr*sp*sy+cr*cy;m.m[1][2]=-sr*cp;m.m[1][3]=0.f;
    m.m[2][0]=-(cr*sp*cy+sr*sy);m.m[2][1]=cy*sr-cr*sp*sy;m.m[2][2]=cr*cp;m.m[2][3]=0.f;
    m.m[3][0]=m.m[3][1]=m.m[3][2]=0.f;m.m[3][3]=1.f;return m;
}
inline int W2S(const SDK::FVector& w,const SDK::FVector& cl,const SDK::FRotator& cr,float fv,int sw,int sh,SDK::FVector2D& o){
    SDK::D3DMATRIX m=RotationMatrix(cr);SDK::FVector ax(m.m[0][0],m.m[0][1],m.m[0][2]),ay(m.m[1][0],m.m[1][1],m.m[1][2]),az(m.m[2][0],m.m[2][1],m.m[2][2]);
    SDK::FVector d=w-cl,t(d.Dot(ay),d.Dot(az),d.Dot(ax));if(t.Z<1.f)t.Z=1.f;
    float cx=(float)sw/2.f,cy=(float)sh/2.f,ft=(float)tan(fv*PI/360.f);
    o.X=cx+t.X*(cx/ft)/t.Z;o.Y=cy-t.Y*(cx/ft)/t.Z;return o.X>=0&&o.X<=sw&&o.Y>=0&&o.Y<=sh?1:0;
}
}
