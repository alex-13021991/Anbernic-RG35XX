// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "SexyAppFramework/graphics/GLImage.h"
#include "SexyAppFramework/graphics/TriVertex.h"
#include "SexyAppFramework/misc/SexyMatrix.h"
#include <cmath>
#include <cstdlib>
#include <vector>
namespace Rg35xx {
inline int renderWidth() {
    const char* env=std::getenv("PVZ_RENDER_WIDTH");
    int w=env ? std::atoi(env) : 800;
    return (w==320 || w==400 || w==480 || w==640 || w==800) ? w : 400;
}
// The scene and widgets keep their 800x600 coordinates. Only rasterization
// is projected onto this smaller backing image; source assets remain intact.
class LowResScreen final : public Sexy::GLImage {
    Sexy::MemoryImage target_;
    float scale_;
    Sexy::Rect rect(const Sexy::Rect& r) const {
        int l=std::floor(r.mX*scale_),t=std::floor(r.mY*scale_);
        int right=std::ceil((r.mX+r.mWidth)*scale_);
        int bottom=std::ceil((r.mY+r.mHeight)*scale_);
        return Sexy::Rect(l,t,right-l,bottom-t);
    }
    Sexy::Rect full() const { return Sexy::Rect(0,0,target_.mWidth,target_.mHeight); }
    Sexy::SexyMatrix3 matrix(const Sexy::SexyMatrix3& m) const {
        Sexy::SexyMatrix3 r=m;
        for(int y=0;y<2;y++)for(int x=0;x<3;x++)r.m[y][x]*=scale_;
        return r;
    }
public:
    LowResScreen(Sexy::GLInterface* gl,int width) : GLImage(gl),target_(mApp),scale_(width/800.0f) {
        mWidth=800;mHeight=600;
        SetImageMode(false,false);
        target_.Create(width,width*3/4);
        target_.SetImageMode(false,false);
    }
    Sexy::MemoryImage& target() { return target_; }
    void drawCachedFrame(const uint16_t* src,int pitch,int sx,int sy,int x,int y,int width,int height,bool chilled) {
        Sexy::Rect out=rect(Sexy::Rect(x,y,width,height));
        Sexy::Rect clipped=out.Intersection(full());
        uint32_t* bits=target_.GetBits();
        if(out.mWidth<=0 || out.mHeight<=0)return;
        for(int dy=clipped.mY;dy<clipped.mY+clipped.mHeight;dy++) {
            int sourceY=sy+(dy-out.mY)*height/out.mHeight;
            auto* row=bits+dy*target_.mWidth;
            for(int dx=clipped.mX;dx<clipped.mX+clipped.mWidth;dx++) {
                int sourceX=sx+(dx-out.mX)*width/out.mWidth;
                uint16_t v=src[sourceY*pitch+sourceX];
                unsigned a=(v>>12)*17;
                if(!a)continue;
                unsigned r=((v>>8)&15)*17,g=((v>>4)&15)*17,b=(v&15)*17;
                auto div255=[](unsigned p){return (p+1+(p>>8))>>8;};
                if(chilled){r=div255(r*75);g=div255(g*75);}
                if(a!=255) {
                    unsigned d=row[dx],inv=255-a;
                    r=div255(r*a+((d>>16)&255)*inv);
                    g=div255(g*a+((d>>8)&255)*inv);
                    b=div255(b*a+(d&255)*inv);
                }
                row[dx]=0xff000000|(r<<16)|(g<<8)|b;
            }
        }
        target_.BitsChanged();
    }

    void Clear() override { target_.Clear(); }
    void ClearRect(const Sexy::Rect& r) override { target_.ClearRect(rect(r)); }
    void FillRect(const Sexy::Rect& r,const Sexy::Color& c,int mode) override { target_.FillRect(rect(r),c,mode); }
    void DrawLine(double x,double y,double x2,double y2,const Sexy::Color& c,int mode) override {
        target_.DrawLine(x*scale_,y*scale_,x2*scale_,y2*scale_,c,mode);
    }
    void DrawLineAA(double x,double y,double x2,double y2,const Sexy::Color& c,int mode) override {
        target_.DrawLineAA(x*scale_,y*scale_,x2*scale_,y2*scale_,c,mode);
    }
    void Blt(Sexy::Image* image,int x,int y,const Sexy::Rect& src,const Sexy::Color& c,int mode,bool=true) override {
        if(!image)return;
        target_.StretchBlt(image,rect(Sexy::Rect(x,y,src.mWidth,src.mHeight)),src,full(),c,mode,true);
    }
    void BltF(Sexy::Image* image,float x,float y,const Sexy::Rect& src,const Sexy::Rect& clip,const Sexy::Color& c,int mode) override {
        if(!image)return;
        int l=std::floor(x*scale_),t=std::floor(y*scale_);
        Sexy::Rect dst(l,t,int(std::ceil((x+src.mWidth)*scale_))-l,int(std::ceil((y+src.mHeight)*scale_))-t);
        target_.StretchBlt(image,dst,src,rect(clip),c,mode,true);
    }
    void StretchBlt(Sexy::Image* image,const Sexy::Rect& dst,const Sexy::Rect& src,const Sexy::Rect& clip,const Sexy::Color& c,int mode,bool fast) override {
        if(image)target_.StretchBlt(image,rect(dst),src,rect(clip),c,mode,fast);
    }
    void BltMirror(Sexy::Image* image,int x,int y,const Sexy::Rect& src,const Sexy::Color& c,int mode,bool=true) override {
        if(!image)return;
        Sexy::SexyMatrix3 m;m.LoadIdentity();m.m00=-1;
        BltMatrix(image,x+src.mWidth*0.5f,y+src.mHeight*0.5f,m,Sexy::Rect(0,0,800,600),c,mode,src,true);
    }
    void StretchBltMirror(Sexy::Image* image,const Sexy::Rect& dst,const Sexy::Rect& src,const Sexy::Rect& clip,const Sexy::Color& c,int mode,bool fast) override {
        if(!image || src.mWidth<=0 || src.mHeight<=0)return;
        Sexy::SexyMatrix3 m;m.LoadIdentity();
        m.m00=-float(dst.mWidth)/src.mWidth;m.m11=float(dst.mHeight)/src.mHeight;
        BltMatrix(image,dst.mX+dst.mWidth*0.5f,dst.mY+dst.mHeight*0.5f,m,clip,c,mode,src,true);
    }
    void BltMatrix(Sexy::Image* image,float x,float y,const Sexy::SexyMatrix3& m,const Sexy::Rect& clip,const Sexy::Color& c,int mode,const Sexy::Rect& src,bool blend) override {
        if(image)target_.BltMatrix(image,x*scale_,y*scale_,matrix(m),rect(clip),c,mode,src,blend);
    }
    void BltRotated(Sexy::Image* image,float x,float y,const Sexy::Rect& src,const Sexy::Rect& clip,const Sexy::Color& c,int mode,double angle,float cx,float cy) override {
        if(!image)return;
        Sexy::SexyMatrix3 m;m.LoadIdentity();
        float co=std::cos(angle),si=std::sin(angle);
        m.m00=co;m.m01=si;m.m10=-si;m.m11=co;
        float dx=src.mWidth*0.5f-cx,dy=src.mHeight*0.5f-cy;
        m.m02=cx+co*dx+si*dy;m.m12=cy+co*dy-si*dx;
        BltMatrix(image,x,y,m,clip,c,mode,src,true);
    }
    void BltTrianglesTex(Sexy::Image* image,const Sexy::TriVertex v[][3],int n,const Sexy::Rect& clip,const Sexy::Color& c,int mode,float tx,float ty,bool blend) override {
        if(!image || n<=0)return;
        // Keep triangles consecutive without changing UVs or vertex colors.
        std::vector<Sexy::TriVertex> vertices(n*3);
        for(int i=0;i<n;i++)for(int j=0;j<3;j++){
            auto& d=vertices[i*3+j];d=v[i][j];d.x*=scale_;d.y*=scale_;
        }
        target_.BltTrianglesTex(image,reinterpret_cast<const Sexy::TriVertex(*)[3]>(vertices.data()),n,rect(clip),c,mode,tx*scale_,ty*scale_,blend);
    }
    void BltTrianglesTexDirect(Sexy::Image* image,const Sexy::TriVertex v[][3],int n,const Sexy::Rect& clip,const Sexy::Color& c,int mode,float tx,float ty,bool blend) override {
        BltTrianglesTex(image,v,n,clip,c,mode,tx,ty,blend);
    }
    bool PolyFill3D(const Sexy::Point v[],int n,const Sexy::Rect* clip,const Sexy::Color& c,int mode,int tx,int ty) override {
        if(n<=0)return false;
        std::vector<Sexy::Point> points(n);
        for(int i=0;i<n;i++)points[i]=Sexy::Point(std::round((v[i].mX+tx)*scale_),std::round((v[i].mY+ty)*scale_));
        Sexy::Rect cl=clip?rect(*clip):full();
        return target_.PolyFill3D(points.data(),n,&cl,c,mode,0,0);
    }
};
}
