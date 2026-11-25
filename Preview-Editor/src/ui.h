#ifndef PREVIEW_UI_H
#define PREVIEW_UI_H
#include "browser.h"
#include "theme.h"
enum { BG=0x162121,PANEL=0x233232,LINE=0x43605a,INK=0xf1f5ef,MUTED=0xb2c4be,
       ACCENT=0x99e4b4,SELECTED=0x295041,AMBER=0xffcf82 };
typedef enum { NONE,UP,DOWN,LEFT,RIGHT,A,B,X,Y,L,R,MENU,QUIT } Key;
extern SDL_Surface *screen;
extern TTF_Font *title_font,*font,*small;
extern Pixel canvas[CW*CH];
int ui_init(void);
void ui_close(void);
Key read_key(void);
void box(int x,int y,int w,int h,int color);
void label(int x,int y,int width,TTF_Font *f,int color,const char *s);
void header(const char *s,const char *right);
void footer(const char *first,const char *second);
void list_row(int i,int selected,const char *left,const char *right);
void draw_art(const Image *img,Placement p,const Theme *t,Prefs prefs,int full,const Browser *b);
#endif
