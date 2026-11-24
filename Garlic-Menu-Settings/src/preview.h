#ifndef PREVIEW_H
#define PREVIEW_H
#include "gms.h"
#include <SDL.h>
#include <SDL_ttf.h>
int preview_init(SDL_Surface *video,const char *assets);
void preview_draw(SDL_Surface *video,Settings s,int cover,int right,int full,unsigned *start);
void preview_free(void);
#endif

