// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Rg35xx {
class Input {
    int fd = -1;
    int hatX = 0, hatY = 0;
    bool leftTrigger = false, rightTrigger = false;
    static void key(Uint32 window, SDL_Keycode symbol, bool pressed) {
        SDL_Event e{};
        e.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
        e.key.windowID = window;
        e.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
        e.key.keysym.sym = symbol;
        e.key.keysym.scancode = SDL_GetScancodeFromKey(symbol);
        SDL_PushEvent(&e);
    }
    static void axis(Uint32 window, int& state, int next, SDL_Keycode minus, SDL_Keycode plus) {
        if (state == next) return;
        if (state) key(window, state < 0 ? minus : plus, false);
        state = next;
        if (state) key(window, state < 0 ? minus : plus, true);
    }
    static void volume(int step) {
        const char* path = "/sys/class/volume/value";
        FILE* f = fopen(path, "r");
        if (!f) return;
        int value;
        bool valid = fscanf(f, "%d", &value) == 1 && value >= 0 && value <= 40;
        fclose(f);
        if (!valid) return;
        value += step;
        if (value < 0 || value > 40) return;
        if ((f = fopen(path, "w"))) { fprintf(f, "%d\n", value); fclose(f); }
    }
public:
    Input() {
        DIR* dir = opendir("/dev/input");
        if (dir) {
            while (auto* e = readdir(dir)) {
                if (strncmp(e->d_name,"event",5)) continue;
                int candidate = openat(dirfd(dir), e->d_name, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
                char name[128]{};
                if (candidate < 0) continue;
                if (ioctl(candidate, EVIOCGNAME(sizeof(name)-1), name) >= 0 &&
                    strcmp(name,"RG35XX Gamepad") == 0) { fd=candidate; break; }
                close(candidate);
            }
            closedir(dir);
        }
        if (fd < 0) fprintf(stderr,"RG35XX Gamepad unavailable\n");
    }
    ~Input() { if (fd >= 0) close(fd); }
    void poll(Uint32 window) {
        input_event e;
        while (fd >= 0 && read(fd,&e,sizeof(e)) == sizeof(e)) {
            if (e.type == EV_ABS) {
                if (e.code == ABS_HAT0X) axis(window,hatX,e.value,SDLK_LEFT,SDLK_RIGHT);
                if (e.code == ABS_HAT0Y) axis(window,hatY,e.value,SDLK_UP,SDLK_DOWN);
                if (e.code == ABS_Z || e.code == ABS_RZ) {
                    bool& held = e.code == ABS_Z ? leftTrigger : rightTrigger;
                    bool next = e.value > 127;
                    if (next != held) key(window,e.code == ABS_Z ? SDLK_TAB : SDLK_BACKSPACE,next);
                    held = next;
                }
            } else if (e.type == EV_KEY && e.value != 2) {
                SDL_Keycode symbol;
                switch (e.code) {
                    case 0x130: symbol=SDLK_LALT; break;
                    case 0x131: symbol=SDLK_SPACE; break;
                    case 0x133: symbol=SDLK_LSHIFT; break;
                    case 0x134: symbol=SDLK_LCTRL; break;
                    case 0x136: symbol=SDLK_e; break;
                    case 0x137: symbol=SDLK_TAB; break;
                    case 0x13a: symbol=SDLK_RCTRL; break;
                    case 0x13b: symbol=SDLK_RETURN; break;
                    case 0x13c: symbol=SDLK_HOME; break;
                    case 0x13d: if (e.value) volume(1); continue;
                    case 0x13e: if (e.value) volume(-1); continue;
                    default: continue;
                }
                key(window,symbol,e.value != 0);
            }
        }
    }
};
}
