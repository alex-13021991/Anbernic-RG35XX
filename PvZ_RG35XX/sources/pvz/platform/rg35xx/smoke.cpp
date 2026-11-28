#include "Platform.h"
#include <cstdio>
#include <dlfcn.h>
int main() {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) { puts(SDL_GetError()); return 2; }
    auto* w = SDL_CreateWindow("RG35XX PvZ backend",0,0,640,480,0);
    auto* r = w ? SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE) : nullptr;
    if (!r) { puts(SDL_GetError()); return 3; }
    void* mpt = dlopen("libopenmpt.so.0",RTLD_NOW);
    if (!mpt) { puts(dlerror()); return 4; }
    printf("OPENMPT_OK\n"); fflush(stdout);
    bool done=false;
    Uint32 begin=SDL_GetTicks();
    while (!done && SDL_GetTicks()-begin<20000) {
        Rg35xxPoll(SDL_GetWindowID(w));
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_KEYDOWN || e.type==SDL_KEYUP) {
                printf("KEY %d %d\n",e.key.keysym.sym,e.type==SDL_KEYDOWN); fflush(stdout);
                if (e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_HOME) done=true;
            }
        }
        SDL_SetRenderDrawColor(r,255,0,0,255); SDL_RenderClear(r);
        SDL_Rect lower={0,240,640,240};
        SDL_SetRenderDrawColor(r,0,0,255,255); SDL_RenderFillRect(r,&lower);
        Rg35xxPresent(r);
        SDL_Delay(16);
    }
    dlclose(mpt); SDL_DestroyRenderer(r); SDL_DestroyWindow(w); SDL_Quit();
    return done ? 0 : 5;
}
