// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from Jazz2_RG35XX platform by alex-13021991.
#include "Platform.h"
#include "RgFramebuffer.h"
#include "RgInput.h"
#include <cstdlib>
void Rg35xxPoll(Uint32 window) {
    static Rg35xx::Input input;
    input.poll(window);
}
static Rg35xx::Framebuffer& display() {
    static Rg35xx::Framebuffer fb;
    static bool initialized = fb.openDisplay();
    if (!initialized) { fprintf(stderr,"Cannot open RG35XX display"); std::exit(2); }
    return fb;
}
void Rg35xxPresentPixels(const uint16_t* pixels) {
    display().present(pixels,640,480,1280);
}
void Rg35xxPresent(SDL_Renderer* renderer) {
    static uint16_t pixels[640*480];
    if (SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_RGB565,pixels,640*2) != 0) {
        fprintf(stderr,"SDL readback: %s\n",SDL_GetError()); std::exit(2);
    }
    Rg35xxPresentPixels(pixels);
    SDL_RenderPresent(renderer);
}

const Uint8* Rg35xxKeyboardState() { return Rg35xx::keyboardState; }

void Rg35xxPresentScaledPixels(const uint16_t* pixels,int width,int height) {
    display().present(pixels,width,height,width*2);
}
