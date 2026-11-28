#include "SexyAppFramework/graphics/GLInterface.h"
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include "platform/rg35xx/LowResScreen.h"
#include "LawnApp.h"
#include <cstdio>
#include <cstdlib>
using namespace Sexy;
static void require(bool x,const char*msg){if(!x){fprintf(stderr,"FAIL: %s",msg);fputc(10,stderr);std::exit(1);}}
static void crash(int,siginfo_t*,void* context){
 auto*u=static_cast<ucontext_t*>(context);
 fprintf(stderr,"pc=%lx lr=%lx",(unsigned long)u->uc_mcontext.arm_pc,(unsigned long)u->uc_mcontext.arm_lr);fputc(10,stderr);_exit(8);
}
int main(){
 struct sigaction sa{};sa.sa_sigaction=crash;sa.sa_flags=SA_SIGINFO;sigaction(SIGSEGV,&sa,nullptr);

 setvbuf(stdout,nullptr,_IONBF,0);puts("construct app");
 gLawnApp=new LawnApp();gLawnApp->mGLInterface=std::make_unique<GLInterface>(gLawnApp);puts("construct screen");
 {
 GLImage probe(nullptr);puts("base ready");
 Rg35xx::LowResScreen screen(nullptr,400);
 puts("screen ready");auto& target=screen.target();
 auto pixel=[&](int x,int y){return target.GetBits()[y*400+x]&0xffffff;};
 require(screen.mWidth==800 && screen.mHeight==600,"logical bounds");
 require(target.mWidth==400 && target.mHeight==300,"physical bounds");
 screen.FillRect(Rect(0,0,800,600),Color::Black,0);
 screen.FillRect(Rect(20,40,80,100),Color(255,0,0),0);
 require(pixel(10,20)==0xff0000 && pixel(49,69)==0xff0000,"rectangle projection");
 require(pixel(9,20)==0 && pixel(50,69)==0,"rectangle bounds");
 puts("source");MemoryImage source(gLawnApp);source.Create(8,8);source.SetImageMode(false,false);
 source.FillRect(Rect(0,0,4,4),Color(255,0,0),0);
 source.FillRect(Rect(4,0,4,4),Color(0,255,0),0);
 source.FillRect(Rect(0,4,4,4),Color(0,0,255),0);
 source.FillRect(Rect(4,4,4,4),Color::White,0);
 screen.Blt(&source,100,200,Rect(0,0,8,8),Color::White,0);
 require(pixel(50,100)==0xff0000 && pixel(53,100)==0x00ff00,"sprite top");
 require(pixel(50,103)==0x0000ff && pixel(53,103)==0xffffff,"sprite bottom");
 screen.BltMirror(&source,200,200,Rect(0,0,8,8),Color::White,0);
 require(pixel(100,100)==0x00ff00 && pixel(103,100)==0xff0000,"mirror");
 SexyMatrix3 m;m.LoadIdentity();
 puts("matrix");screen.BltMatrix(&source,300,200,m,Rect(0,0,800,600),Color::White,0,Rect(0,0,8,8),true);
 require(pixel(148,98)==0xff0000 && pixel(151,98)==0x00ff00,"matrix top");
 require(pixel(148,101)==0x0000ff && pixel(151,101)==0xffffff,"matrix bottom");
 puts("rotated");screen.BltRotated(&source,400,200,Rect(0,0,8,8),Rect(0,0,800,600),Color::White,0,0,0,0);
 require(pixel(200,100)==0xff0000 && pixel(203,103)==0xffffff,"rotation zero");
 screen.BltRotated(&source,500,200,Rect(0,0,8,8),Rect(0,0,800,600),Color::White,0,1.5707963267948966,4,4);
 require(pixel(250,100)==0x00ff00 && pixel(253,100)==0xffffff,"quarter rotation top");
 require(pixel(250,103)==0xff0000 && pixel(253,103)==0x0000ff,"quarter rotation bottom");
 TriVertex tri[2][3]={
  {TriVertex(600,200,0,0),TriVertex(608,200,1,0),TriVertex(600,208,0,1)},
  {TriVertex(608,200,1,0),TriVertex(608,208,1,1),TriVertex(600,208,0,1)}
 };
 screen.BltTrianglesTex(&source,tri,2,Rect(0,0,800,600),Color::White,0,0,0,true);
 require(pixel(300,100)==0xff0000 && pixel(303,103)==0xffffff,"triangle projection");
 uint16_t cached[64];
 for(int y=0;y<8;y++)for(int x=0;x<8;x++)cached[y*8+x]=(y<4 ? (x<4?0xff00:0xf0f0) : (x<4?0xf00f:0xffff));
 screen.drawCachedFrame(cached,8,0,0,100,300,8,8,false);
 require(pixel(50,150)==0xff0000 && pixel(53,153)==0xffffff,"cached plant projection");
 screen.ClearRect(Rect(20,40,80,100));
 require(pixel(10,20)==0,"clear projection");
 }
 puts("PASS: low-resolution logical bounds, raster bounds, rectangles, sprites, mirrors, matrix, rotation and clearing");
 return 0;
}
