// SPDX-License-Identifier: GPL-3.0-or-later
#include "../platform/RgFramebuffer.h"
#include <cstdint>
#ifndef TEST_VIDEO_WIDTH
#define TEST_VIDEO_WIDTH 320
#endif
#ifndef TEST_VIDEO_HEIGHT
#define TEST_VIDEO_HEIGHT 240
#endif
int main(){
 constexpr unsigned Width=TEST_VIDEO_WIDTH, Height=TEST_VIDEO_HEIGHT;
 const size_t page=sysconf(_SC_PAGESIZE), stride=2*page;
 if(Width*2>page)return 2;
 void* storage=mmap(nullptr,stride*Height,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 if(storage==MAP_FAILED)return 2;
 for(unsigned y=0;y<Height;++y){
  auto* writable=static_cast<uint8_t*>(storage)+y*stride;
  if(mprotect(writable,page,PROT_READ|PROT_WRITE)<0)return 2;
  auto* row=reinterpret_cast<uint16_t*>(writable+page-Width*2);
  for(unsigned x=0;x<Width;++x)row[x]=uint16_t(y*Width+x);
 }
 Rg35xx::Framebuffer fb;
 if(!fb.openDisplay())return 1;
 fb.present(static_cast<uint8_t*>(storage)+page-Width*2,Width,Height,stride);
 munmap(storage,stride*Height);
 return 0;
}
