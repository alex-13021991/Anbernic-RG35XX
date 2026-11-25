#ifndef PREVIEW_THEME_H
#define PREVIEW_THEME_H
#include "editor.h"
#include <SDL.h>
#include <SDL_ttf.h>
typedef struct {
    Image background,overlay,buttons[4],battery;
    int top,bottom,alignment,margin,width,active,inactive,guide,labels;
    TTF_Font *list_font,*guide_font;
} Theme;
void theme_load(Theme *t,const char *tf1);
void theme_free(Theme *t);
void theme_panels(const Theme *t,Prefs p,int *top,int *bottom);
#endif
