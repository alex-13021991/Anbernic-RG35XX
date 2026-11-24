/* Only linked into the test executable. Exercise the SDL event loop with a dummy display. */
static void test_frame(SDL_Surface *video,Page page) {
    static int frame;
    const char *keys=getenv("GMS_KEYS"),*out=getenv("GMS_FRAMES");
    if (out) {
        char path[PATH_MAX];
        snprintf(path,sizeof path,"%s/%02d-%d.bmp",out,frame,(int)page);
        if (SDL_SaveBMP(video,path)<0) exit(3);
    }
    if (!keys || frame>=(int)strlen(keys)) { stop=1; return; }
    int c=keys[frame++];
    SDL_Event e={0}; e.type=SDL_KEYUP; e.key.keysym.sym=(SDLKey)c; SDL_PushEvent(&e);
    e.type=SDL_KEYDOWN; SDL_PushEvent(&e);
}

