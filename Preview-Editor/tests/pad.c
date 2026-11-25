/* Script SDL key events in the unmodified ARM release. A '*' captures the
   current video surface at the path provided by the test harness. */
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

int SDL_PollEvent(SDL_Event *e) {
    static int pos,release;
    const char *keys=getenv("PREVIEW_KEYS");
    if (keys && keys[pos]=='*') {
        SDL_SaveBMP(SDL_GetVideoSurface(),getenv("PREVIEW_FRAME"));
        pos++;
    }
    memset(e,0,sizeof *e);
    if (!keys || !keys[pos]) { e->type=SDL_QUIT; return 1; }
    e->type=release ? SDL_KEYUP : SDL_KEYDOWN;
    e->key.keysym.sym=(SDLKey)(unsigned char)keys[pos];
    if (release) pos++;
    release=!release;
    return 1;
}
