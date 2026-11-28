// SPDX-License-Identifier: GPL-3.0-or-later
#include "../platform/RgFramebuffer.h"
#include "../platform/RgInput.h"
#include <ctime>
#include <signal.h>

#ifndef TEST_VIDEO_WIDTH
#define TEST_VIDEO_WIDTH 304
#endif
static constexpr int Width = TEST_VIDEO_WIDTH;
static uint16_t pixels[228][Width];
static void timedOut(int) {}
static double seconds() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    Rg35xx::Framebuffer screen;
    if (!screen.openDisplay()) return 1;
    if (argc == 2 && strcmp(argv[1],"--timing") == 0) {
        screen.probeTiming();
        int fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
        if (fd < 0) return 1;
        struct sigaction sa{};
        sa.sa_handler = timedOut;
        sigaction(SIGALRM,&sa,nullptr);
        unsigned crtc = 0;
        alarm(1);
        const double before = seconds();
        errno = 0;
        int ret = ioctl(fd, FBIO_WAITFORVSYNC, &crtc);
        int error = errno;
        alarm(0);
        printf("FBIO_WAITFORVSYNC rc=%d errno=%d elapsed_ms=%.2f\n",ret,error,(seconds()-before)*1000);
        close(fd);
    }
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    Rg35xx::Input pad;
    const uint16_t bars[] = {0xf800,0x07e0,0x001f,0xffe0,0xf81f,0x07ff,0xffff,0};
    for (int y=0;y<228;++y)
        for (int x=0;x<Width;++x)
            pixels[y][x] = (y>207 && x<20) ? 0xffff : bars[x/(Width/8)];
    auto start = seconds();
    unsigned frames = 0;
    bool done = false;
    while (!done && seconds()-start < 5) {
        pad.poll(0);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                printf("key=%s\n",SDL_GetKeyName(event.key.keysym.sym));
                if (event.key.keysym.sym == SDLK_HOME) done = true;
            }
        }
        screen.present(pixels,Width,228,sizeof(pixels[0]));
        ++frames;
    }
    printf("frames=%u elapsed_s=%.3f copy_fps=%.1f\n",frames,seconds()-start,frames/(seconds()-start));
    SDL_Quit();
}
