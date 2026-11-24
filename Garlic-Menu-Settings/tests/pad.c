/* Supply scripted key presses to the unmodified release ELF under QEMU. */
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
int SDL_PollEvent(SDL_Event *e) {
    static int pos,release;
    const char *keys=getenv("GMS_PAD");
    memset(e,0,sizeof *e);
    if (!keys || !keys[pos]) { e->type=SDL_QUIT; return 1; }
    e->type=release ? SDL_KEYUP : SDL_KEYDOWN;
    e->key.keysym.sym=(SDLKey)(unsigned char)keys[pos];
    if (release) pos++;
    release=!release;
    return 1;
}

