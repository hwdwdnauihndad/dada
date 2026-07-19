#pragma once
#include <Windows.h>
extern "C" { double __cdecl sqrt(double); double __cdecl sin(double); double __cdecl cos(double); double __cdecl tan(double); double __cdecl atan2(double,double); }
namespace SDK {
struct FVector { float X,Y,Z; FVector():X(0),Y(0),Z(0){} FVector(float x,float y,float z):X(x),Y(y),Z(z){}
    FVector operator+(const FVector&o)const{return{X+o.X,Y+o.Y,Z+o.Z};} FVector operator-(const FVector&o)const{return{X-o.X,Y-o.Y,Z-o.Z};}
    float Dot(const FVector&o)const{return X*o.X+Y*o.Y+Z*o.Z;}
    float Size()const{return (float)sqrt(X*X+Y*Y+Z*Z);} float Dist(const FVector&o)const{float dx=o.X-X,dy=o.Y-Y,dz=o.Z-Z;return (float)sqrt(dx*dx+dy*dy+dz*dz);}
    int Normalize(float t=1e-8f){float s=Size();if(s<t)return 0;float r=1.f/s;X*=r;Y*=r;Z*=r;return 1;}
};
struct FVector2D{float X,Y;FVector2D():X(0),Y(0){}FVector2D(float x,float y):X(x),Y(y){}};
struct FRotator{float Pitch,Yaw,Roll;FRotator():Pitch(0),Yaw(0),Roll(0){}FRotator(float p,float y,float r):Pitch(p),Yaw(y),Roll(r){}};
struct FTransform{double Rot[4];double Trans[3];char Pad1[8];double Scale[3];char Pad2[8];};
struct D3DMATRIX{float m[4][4];};
inline D3DMATRIX MatrixMultiply(const D3DMATRIX& a,const D3DMATRIX& b){D3DMATRIX r{};for(int i=0;i<4;i++)for(int j=0;j<4;j++)for(int k=0;k<4;k++)r.m[i][j]+=a.m[i][k]*b.m[k][j];return r;}
inline D3DMATRIX TransformToMatrix(const FTransform& t){D3DMATRIX m{};double x2=t.Rot[0]+t.Rot[0],y2=t.Rot[1]+t.Rot[1],z2=t.Rot[2]+t.Rot[2];double xx2=t.Rot[0]*x2,yy2=t.Rot[1]*y2,zz2=t.Rot[2]*z2,yz2=t.Rot[1]*z2,wx2=t.Rot[3]*x2,xy2=t.Rot[0]*y2,wz2=t.Rot[3]*z2,xz2=t.Rot[0]*z2,wy2=t.Rot[3]*y2;float sx=(float)(t.Scale[0]==0.0?1.0:t.Scale[0]),sy=(float)(t.Scale[1]==0.0?1.0:t.Scale[1]),sz=(float)(t.Scale[2]==0.0?1.0:t.Scale[2]);m.m[0][0]=(float)((1.0-(yy2+zz2))*sx);m.m[0][1]=(float)((xy2+wz2)*sx);m.m[0][2]=(float)((xz2-wy2)*sx);m.m[0][3]=0.f;m.m[1][0]=(float)((xy2-wz2)*sy);m.m[1][1]=(float)((1.0-(xx2+zz2))*sy);m.m[1][2]=(float)((yz2+wx2)*sy);m.m[1][3]=0.f;m.m[2][0]=(float)((xz2+wy2)*sz);m.m[2][1]=(float)((yz2-wx2)*sz);m.m[2][2]=(float)((1.0-(xx2+yy2))*sz);m.m[2][3]=0.f;m.m[3][0]=(float)t.Trans[0];m.m[3][1]=(float)t.Trans[1];m.m[3][2]=(float)t.Trans[2];m.m[3][3]=1.f;return m;}
}
