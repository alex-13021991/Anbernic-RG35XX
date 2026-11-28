#pragma once
#include <SDL.h>
void Rg35xxPoll(Uint32 window);
void Rg35xxPresent(SDL_Renderer* renderer);

const Uint8* Rg35xxKeyboardState();

void Rg35xxPresentPixels(const uint16_t* pixels);

void Rg35xxPresentScaledPixels(const uint16_t* pixels,int width,int height);
