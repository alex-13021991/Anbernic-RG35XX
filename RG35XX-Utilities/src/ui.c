#include "ui.h"
#include "core.h"
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
SDL_Surface *screen;
TTF_Font *font, *small, *title;
volatile sig_atomic_t stopping;
static unsigned char held[SDLK_LAST];
/* Match Dingux Commander's dark slate palette. */
const SDL_Color C_TEXT={204,204,204,0}, C_MUTED={150,160,170,0},
 C_ACCENT={150,170,188,0}, C_DARK={18,22,28,0}, C_RED={190,105,105,0};
#define C_BG 0x12161c
#define C_BG_ALT 0x191f27
#define C_BAR 0x485765
#define C_CURSOR 0x305265
#define C_BORDER 0x485765
#define C_ERROR 0x803737
static void stop_handler(int sig) { (void)sig; stopping=1; }
int ui_init(const char *caption) {
    putenv("SDL_NOMOUSE=1");
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) goto fail;
    SDL_ShowCursor(SDL_DISABLE);
    screen=SDL_SetVideoMode(640,480,16,SDL_SWSURFACE);
    if (!screen || TTF_Init() < 0) goto fail;
    font=TTF_OpenFont("res/FreeSans.ttf",22);
    small=TTF_OpenFont("res/FreeSans.ttf",18);
    title=TTF_OpenFont("res/FreeSans.ttf",28);
    if (!font || !small || !title) goto fail;
    SDL_WM_SetCaption(caption,caption);
    SDL_EnableKeyRepeat(320,100);
    signal(SIGTERM,stop_handler); signal(SIGINT,stop_handler);
    return 1;
fail:
    fprintf(stderr,"UI initialization: %s / %s\n",SDL_GetError(),TTF_GetError());
    ui_close(); return 0;
}
void ui_close(void) {
    if(font) TTF_CloseFont(font); if(small) TTF_CloseFont(small);
    if(title) TTF_CloseFont(title);
    font=small=title=NULL; TTF_Quit(); SDL_Quit();
}
void box(int x,int y,int w,int h,unsigned rgb) {
    SDL_Rect r={(Sint16)x,(Sint16)y,(Uint16)w,(Uint16)h};
    SDL_FillRect(screen,&r,SDL_MapRGB(screen->format,rgb>>16,(rgb>>8)&255,rgb&255));
}
void outline(int x,int y,int w,int h,unsigned c) {
    box(x,y,w,2,c); box(x,y+h-2,w,2,c); box(x,y,2,h,c); box(x+w-2,y,2,h,c);
}
void text(int x,int y,const char *s,TTF_Font *f,SDL_Color c,int maxw) {
    char buf[1024]; int w=0,h=0; size_t n;
    snprintf(buf,sizeof(buf),"%s",s); n=strlen(buf);
    while(n && (TTF_SizeUTF8(f,buf,&w,&h)<0 || w>maxw)) {
        n=utf8_previous(buf,n); buf[n]=0;
    }
    if(n<strlen(s) && n>3) {
        do { n=utf8_previous(buf,n); buf[n]=0;
             strncat(buf,"...",sizeof(buf)-strlen(buf)-1);
             if(TTF_SizeUTF8(f,buf,&w,&h)==0 && w<=maxw) break;
             buf[n]=0;
        } while(n);
    }
    if(!buf[0]) return;
    SDL_Surface *t=TTF_RenderUTF8_Blended(f,buf,c);
    if(t) { SDL_Rect dst={(Sint16)x,(Sint16)y,0,0}; SDL_BlitSurface(t,NULL,screen,&dst); SDL_FreeSurface(t); }
}
void center(int x,int y,int w,const char *s,TTF_Font *f,SDL_Color c) {
    int tw=0,th; TTF_SizeUTF8(f,s,&tw,&th);
    text(x+(tw<w?(w-tw)/2:0),y,s,f,c,w);
}
void page(const char *heading,const char *right) {
    box(0,0,640,480,C_BG_ALT);
    box(0,0,640,52,C_BAR); text(18,12,heading,title,C_TEXT,470);
    if(right) text(495,20,right,small,C_TEXT,130);
    box(18,52,604,2,C_BORDER);
}
void footer(const char *hint) {
    box(0,445,640,35,C_BAR); text(18,452,hint,small,C_TEXT,604);
}
void button(int x,int y,int w,const char *s,int selected,int danger) {
    box(x,y,w,42,selected?C_CURSOR:C_BG);
    if(!selected) outline(x,y,w,42,danger?C_ERROR:C_BORDER);
    center(x+4,y+8,w-8,s,font,selected?C_TEXT:(danger?C_RED:C_TEXT));
}
void flip(void) { SDL_Flip(screen); }
int key(void) {
    SDL_Event e;
    if(stopping) return KEY_QUIT;
    while(SDL_PollEvent(&e)) {
        if(e.type==SDL_QUIT) return KEY_QUIT;
        if(e.type==SDL_KEYUP) {
            if(e.key.keysym.sym>=0 && e.key.keysym.sym<SDLK_LAST) held[e.key.keysym.sym]=0;
            continue;
        }
        if(e.type!=SDL_KEYDOWN) continue;
        int sym=e.key.keysym.sym;
        int repeatable=sym==SDLK_w||sym==SDLK_s||sym==SDLK_q||sym==SDLK_d||
            sym==SDLK_UP||sym==SDLK_DOWN||sym==SDLK_LEFT||sym==SDLK_RIGHT||
            sym==SDLK_h||sym==SDLK_j||sym==SDLK_l||sym==SDLK_k||sym==SDLK_PAGEUP||sym==SDLK_PAGEDOWN;
        if(sym>=0 && sym<SDLK_LAST) {if(held[sym]&&!repeatable)continue;held[sym]=1;}
        switch(e.key.keysym.sym) {
            case SDLK_w: case SDLK_UP: return KEY_UP;
            case SDLK_s: case SDLK_DOWN: return KEY_DOWN;
            case SDLK_q: case SDLK_LEFT: return KEY_LEFT;
            case SDLK_d: case SDLK_RIGHT: return KEY_RIGHT;
            case SDLK_a: case SDLK_RETURN: return KEY_A;
            case SDLK_b: case SDLK_ESCAPE: return KEY_B;
            case SDLK_x: return KEY_X; case SDLK_y: return KEY_Y;
            case SDLK_h: case SDLK_j: case SDLK_PAGEUP: return KEY_L;
            case SDLK_l: case SDLK_k: case SDLK_PAGEDOWN: return KEY_R;
            case SDLK_u: return KEY_MENU;
            default: break;
        }
    }
    return KEY_NONE;
}
static void release_keys(void) {
    SDL_Event e; SDL_EnableKeyRepeat(0,0);
    /* Discard queued repeat events before entering a destructive dialog. */
    while(SDL_PollEvent(&e)) {
        if(e.type==SDL_QUIT) stopping=1;
        if(e.type==SDL_KEYUP && e.key.keysym.sym>=0 && e.key.keysym.sym<SDLK_LAST) held[e.key.keysym.sym]=0;
    }
}
int confirm(const char *heading,const char *l1,const char *l2,const char *yes) {
    int chosen=0,result=0,done=0; release_keys();
    while(!done) {
        box(42,120,556,246,C_BG); outline(42,120,556,246,C_BORDER);
        center(58,139,524,heading,title,C_TEXT);
        center(62,189,516,l1,font,C_TEXT);
        center(62,225,516,l2,small,C_MUTED);
        button(76,291,226,"Отмена",!chosen,0); button(328,291,226,yes,chosen,1);
        flip(); int k=key();
        if(k==KEY_LEFT || k==KEY_RIGHT) chosen=!chosen;
        if(k==KEY_B || k==KEY_MENU || k==KEY_QUIT) done=1;
        if(k==KEY_A) { result=chosen; done=1; }
        SDL_Delay(16);
    }
    SDL_EnableKeyRepeat(320,100); return result;
}
void message(const char *heading,const char *l1,const char *l2) {
    release_keys();
    for(;;) {
        box(32,137,576,206,C_BG); outline(32,137,576,206,C_BORDER);
        center(45,154,550,heading,title,C_TEXT);
        center(45,204,550,l1,font,C_TEXT); center(45,239,550,l2,small,C_MUTED);
        center(45,292,550,"A / B — закрыть",small,C_ACCENT); flip();
        int k=key(); if(k==KEY_A || k==KEY_B || k==KEY_QUIT || k==KEY_MENU) break;
        SDL_Delay(16);
    }
    SDL_EnableKeyRepeat(320,100);
}
void show_image(SDL_Surface *im,int x,int y,int w,int h) {
    if(!im || im->w<=0 || im->h<=0) return;
    double scale=(double)w/im->w; if((double)h/im->h<scale) scale=(double)h/im->h;
    int dw=(int)(im->w*scale),dh=(int)(im->h*scale);
    if(dw<1) dw=1; if(dh<1) dh=1;
    SDL_Rect src={0,0,(Uint16)im->w,(Uint16)im->h};
    SDL_Rect dst={(Sint16)(x+(w-dw)/2),(Sint16)(y+(h-dh)/2),(Uint16)dw,(Uint16)dh};
    /* Image surfaces are converted to the screen's 16-bit format at load. */
    SDL_SoftStretch(im,&src,screen,&dst);
}
