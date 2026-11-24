#include "preview.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { BG=0x182321, PANEL=0x24332f, EDGE=0x4b635a, INK=0xf4f6ec,
       MUTED=0xb2c6b8, DIM=0x86948c, GREEN=0xaee3a8, SELECTED=0x325548, AMBER=0xf1c584 };
enum { ALIGN_ROW, MARGIN_ROW, WIDTH_ROW, SCROLL_ROW, COVER_ROW, SIDE_ROW, RESTORE_ROW, ROW_COUNT };
typedef enum { INTRO, INSTALLED, EDIT, FULL, CONFIRM, RESTORED, ERROR_PAGE, REPAIR } Page;
static SDL_Surface *screen;
static TTF_Font *title,*font,*small;
static volatile sig_atomic_t stop;
static void halt(int sig) { (void)sig; stop=1; }
static void box(int x,int y,int w,int h,int rgb) {
    SDL_Rect dst={(Sint16)x,(Sint16)y,(Uint16)w,(Uint16)h};
    SDL_FillRect(screen,&dst,SDL_MapRGB(screen->format,rgb>>16,rgb>>8,rgb));
}
static int label(int x,int y,TTF_Font *f,int rgb,const char *s) {
    if (!s || !*s) return 0;
    SDL_Color c={(Uint8)(rgb>>16),(Uint8)(rgb>>8),(Uint8)rgb,0};
    SDL_Surface *t=TTF_RenderUTF8_Blended(f,s,c);
    if (!t) return 0;
    SDL_Rect dst={(Sint16)x,(Sint16)y,0,0};
    SDL_BlitSurface(t,NULL,screen,&dst); int w=t->w;
    SDL_FreeSurface(t); return w;
}
static void wrapped(int y,const char *text) {
    char words[512],line[512]="";
    snprintf(words,sizeof words,"%s",text);
    char *rest=NULL;
    for (char *w=strtok_r(words," ",&rest);w;w=strtok_r(NULL," ",&rest)) {
        char next[1024]; snprintf(next,sizeof next,"%s%s%s",line,*line?" ":"",w);
        int width,height;
        if (*line && !TTF_SizeUTF8(font,next,&width,&height) && width>570) {
            label(32,y,font,INK,line); y+=32; strcpy(line,w);
        } else snprintf(line,sizeof line,"%.*s",(int)sizeof line-1,next);
    }
    label(32,y,font,INK,line);
}
static void page_header(const char *text) {
    box(0,0,640,480,BG); box(0,53,640,1,EDGE);
    label(18,10,title,INK,text); label(566,19,small,GREEN,"v1.0.1");
}
static void button(int x,int w,const char *s,int on) {
    box(x,410,w,46,on ? GREEN : PANEL);
    label(x+14,420,font,on ? BG : INK,s);
}
static void modal(Page page,int yes,const char *err) {
    page_header("Garlic Menu Settings");
    switch (page) {
    case INTRO:
        label(28,76,title,AMBER,"Установка патча меню");
        label(28,121,font,INK,"Только для оригинальной RG35XX с GarlicOS 1.4.9.");
        label(28,158,font,INK,"CFW/main будет заменён версией Text Clipping v0.5.");
        label(28,195,font,INK,"В CFW/skin/settings.json добавятся недостающие ключи");
        label(32,241,small,GREEN,"text-max-width   313"); label(313,241,small,GREEN,"text-scroll   true");
        label(32,273,small,GREEN,"text-scroll-speed   40"); label(313,273,small,GREEN,"text-scroll-delay   1000");
        label(32,305,small,GREEN,"text-scroll-pause   1000");
        label(28,357,small,MUTED,"Оба исходных файла будут сохранены рядом с расширением .bak.");
        button(28,185,"Выйти",!yes); button(228,382,"Установить патч",yes); break;
    case CONFIRM:
        label(28,84,title,AMBER,"Восстановить исходные файлы?");
        label(28,152,font,INK,"CFW/main.bak   →   CFW/main");
        label(28,191,font,INK,"CFW/skin/settings.json.bak   →   CFW/skin/settings.json");
        wrapped(262,"Изменения темы после установки будут потеряны. После проверки обоих файлов приложение закроется.");
        button(28,185,"Отмена",!yes); button(228,382,"Восстановить",yes); break;
    case REPAIR:
    case ERROR_PAGE:
        label(28,84,title,AMBER,page==REPAIR?"Настройки повреждены":"Операция недоступна");
        wrapped(159,err);
        button(28,185,"Выйти",page==ERROR_PAGE || !yes);
        if (page==REPAIR) button(228,382,"К восстановлению",yes);
        break;
    case INSTALLED:
    case RESTORED:
        label(28,108,title,GREEN,page==INSTALLED?"Патч установлен":"Исходные файлы восстановлены");
        wrapped(189,page==INSTALLED?"Обе резервные копии проверены. Нажмите OK, чтобы перейти к настройкам меню.":"Оба файла проверены. При следующем запуске снова будет предложена установка.");
        button(28,190,"OK",1); break;
    default: break;
    }
}
static const char *align_name(Alignment a) {
    return a==LEFT ? "Слева" : a==RIGHT ? "Справа" : "Центр";
}
static int clipping_enabled(Settings s) {
    return s.width>0 && s.align==LEFT;
}
static int can_scroll(Settings s,int cover) {
    return clipping_enabled(s) && cover;
}
static int next_row(int row,int step,Settings s,int cover) {
    int next=(row+step+ROW_COUNT)%ROW_COUNT;
    if (next==SCROLL_ROW && !can_scroll(s,cover)) next+=step;
    return next;
}
static void editor(Settings s,int cover,int right,int row,unsigned *start) {
    const char *names[]={"Выравнивание","Отступ","Макс. ширина","Прокрутка","Обложка","Положение","Восстановить из копий"};
    char margin[24],width[24],timing[96];
    snprintf(margin,sizeof margin,"%d px",s.margin);
    if (s.width) snprintf(width,sizeof width,"%d px",s.width); else strcpy(width,"Без обрезки");
    int scrolling=can_scroll(s,cover);
    const char *values[]={align_name(s.align),margin,width,scrolling && s.scroll?"Вкл":"Выкл",cover?"Вкл":"Выкл",right?"Справа":"Слева",""};
    page_header("Настройки меню");
    for (int i=0;i<ROW_COUNT;i++) {
        int y=65+i*46;
        int disabled=i==SCROLL_ROW && !scrolling;
        if (i==row) { box(12,y,290,41,SELECTED); box(12,y,3,41,GREEN); }
        label(21,y+10,small,disabled ? DIM : i==RESTORE_ROW ? AMBER : INK,names[i]);
        int w,h;
        if (*values[i] && !TTF_SizeUTF8(small,values[i],&w,&h)) label(292-w,y+10,small,disabled ? DIM : GREEN,values[i]);
    }
    label(318,69,small,MUTED,"Предпросмотр"); box(316,97,308,232,EDGE);
    preview_draw(screen,s,cover,right,0,start);
    label(318,339,small,MUTED,s.align!=LEFT ? "Обрезка v0.5 только слева" :
        !cover ? "Без обложки — полная строка" : !s.width ? "Обрезка отключена" : "Y   На весь экран");
    snprintf(timing,sizeof timing,"%d px/с   ·   %d / %d мс",s.speed,s.delay,s.pause);
    label(318,367,small,MUTED,timing);
    box(0,403,640,1,EDGE);
    label(18,413,small,INK,"↑↓ Выбор     ← → Значение     L1 / R1  ±10 px");
    label(18,446,small,row==RESTORE_ROW ? AMBER : GREEN,
          row==RESTORE_ROW ? "A  Восстановить…" : "A  Сохранить");
    label(191,446,small,GREEN,"B  Выйти       X  Сброс       Y  Превью");
}
static int clamp(int n,int lo,int hi) { return n<lo ? lo : n>hi ? hi : n; }
static void adjust(Settings *s,int *cover,int *right,int row,int dir,int fast) {
    if (fast && row!=MARGIN_ROW && row!=WIDTH_ROW) return;
    int step=dir*(fast?10:1);
    switch (row) {
    case ALIGN_ROW: {
        const Alignment order[]={LEFT,CENTER,RIGHT};
        int i=s->align==LEFT?0:s->align==CENTER?1:2;
        s->align=order[(i+dir+3)%3]; break;
    }
    case MARGIN_ROW: s->margin=clamp(s->margin+step,0,320); break;
    case WIDTH_ROW: s->width=clamp(s->width+step,0,640); break;
    case SCROLL_ROW: if (can_scroll(*s,*cover)) s->scroll=!s->scroll; break;
    case COVER_ROW: *cover=!*cover; break;
    case SIDE_ROW: *right=!*right; break;
    }
}
static SDLKey getkey(void) {
    static unsigned char down[SDLK_LAST];
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type==SDL_QUIT) { stop=1; return SDLK_UNKNOWN; }
        if (e.type!=SDL_KEYDOWN && e.type!=SDL_KEYUP) continue;
        SDLKey k=e.key.keysym.sym;
        if ((unsigned)k>=SDLK_LAST) continue;
        if (e.type==SDL_KEYUP) { down[k]=0; continue; }
        if (down[k] && (k==SDLK_a || k==SDLK_RETURN || k==SDLK_b || k==SDLK_ESCAPE || k==SDLK_y || k==SDLK_x)) continue;
        down[k]=1;
        switch (k) {
        case SDLK_w: return SDLK_UP; case SDLK_s: return SDLK_DOWN;
        case SDLK_q: return SDLK_LEFT; case SDLK_d: return SDLK_RIGHT;
        case SDLK_h: return SDLK_PAGEUP; case SDLK_l: return SDLK_PAGEDOWN;
        case SDLK_a: return SDLK_RETURN; case SDLK_b: return SDLK_ESCAPE;
        default: return k;
        }
    }
    return SDLK_UNKNOWN;
}
static int fonts(void) {
    title=TTF_OpenFont("assets/font.ttf",28); font=TTF_OpenFont("assets/font.ttf",21);
    small=TTF_OpenFont("assets/font.ttf",18);
    return title && font && small;
}
#ifdef GMS_UI_TEST
#include "ui_driver.h"
#endif
int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)<0 || TTF_Init()<0) {
        fprintf(stderr,"%s\n",SDL_GetError()); return 1;
    }
    atexit(SDL_Quit); atexit(TTF_Quit);
    screen=SDL_SetVideoMode(640,480,16,SDL_SWSURFACE);
    if (!screen || !fonts()) { fprintf(stderr,"%s\n",TTF_GetError()); return 1; }
    SDL_ShowCursor(SDL_DISABLE); SDL_EnableKeyRepeat(380,95);
    signal(SIGINT,halt); signal(SIGTERM,halt);
    char assets[PATH_MAX],err[sizeof problem]="";
    Package pkg={0}; Settings s=defaults(); Hash version="";
    int yes=0,row=0,cover=1,right=1,ready=0;
    unsigned start=SDL_GetTicks();
    Page page=ERROR_PAGE;
    const char *cfw="/mnt/mmc/CFW";
#ifdef GMS_UI_TEST
    cfw=getenv("GMS_FIXTURE");
    if (!cfw) return 1;
#endif
    if (realpath("assets",assets) && enter_system(cfw) && recover()) {
        State state=inspect(&s,version);
        if (state==MANAGED) page=EDIT;
        if (state==RESTORE_ONLY) page=REPAIR;
        if (state==FIRST) {
            /* Payload is on the launch card. Resolve its path independently of TF1. */
            int fd=open(".",O_RDONLY|O_DIRECTORY);
            char dir[PATH_MAX];
            snprintf(dir,sizeof dir,"%s",assets);
            char *slash=strrchr(dir,'/'); if (slash) *slash=0;
            if (fd>=0 && !chdir(dir) && load_package(&pkg)) page=INTRO;
            if (fd<0 || fchdir(fd)) { fail("Системная карта недоступна."); page=ERROR_PAGE; }
            if (fd>=0) close(fd);
        }
    }
    snprintf(err,sizeof err,"%s",problem);
    while (!stop) {
        if (page==EDIT && !ready) {
            ready=preview_init(screen,assets);
            start=SDL_GetTicks();
            if (!ready) { page=REPAIR; snprintf(err,sizeof err,"%s",problem); }
        }
        if (page==EDIT) editor(s,cover,right,row,&start);
        else if (page==FULL) preview_draw(screen,s,cover,right,1,&start);
        else modal(page,yes,err);
        SDL_Flip(screen);
#ifdef GMS_UI_TEST
        test_frame(screen,page);
#endif
        SDLKey k=getkey();
        if (k==SDLK_UNKNOWN) { SDL_Delay(page==EDIT || page==FULL ? 32 : 50); continue; }
        if (page==EDIT) {
            int dir=(k==SDLK_LEFT || k==SDLK_PAGEUP) ? -1 : 1;
            if (k==SDLK_UP || k==SDLK_DOWN) row=next_row(row,k==SDLK_UP ? -1 : 1,s,cover);
            else if (k==SDLK_LEFT || k==SDLK_RIGHT || k==SDLK_PAGEUP || k==SDLK_PAGEDOWN) {
                adjust(&s,&cover,&right,row,dir,k==SDLK_PAGEUP || k==SDLK_PAGEDOWN); start=SDL_GetTicks();
            } else if (k==SDLK_x) {
                Settings d=defaults(); s.align=d.align; s.margin=d.margin; s.width=d.width; s.scroll=d.scroll;
                start=SDL_GetTicks();
            } else if (k==SDLK_y) page=FULL;
            else if (k==SDLK_ESCAPE) stop=1;
            else if (k==SDLK_RETURN && row==RESTORE_ROW) { page=CONFIRM; yes=0; }
            else if (k==SDLK_RETURN) {
                /* Cover visibility is only a preview option. */
                if (!clipping_enabled(s)) s.scroll=0;
                if (save(&s,version)) stop=1; else page=ERROR_PAGE;
            }
        } else if (page==FULL) {
            if (k==SDLK_ESCAPE || k==SDLK_y) page=EDIT;
        } else if (k==SDLK_RIGHT || k==SDLK_LEFT) yes=!yes;
        else if (k==SDLK_ESCAPE) {
            if (page==CONFIRM) page=ready?EDIT:REPAIR; else stop=1;
            yes=0;
        } else if (k==SDLK_RETURN) {
            switch (page) {
            case INTRO:
                if (!yes) stop=1; else page=install(&pkg) ? INSTALLED : ERROR_PAGE;
                break;
            case INSTALLED: page=inspect(&s,version)==MANAGED ? EDIT : ERROR_PAGE; break;
            case CONFIRM: page=!yes ? (ready?EDIT:REPAIR) : restore() ? RESTORED : ERROR_PAGE; break;
            case REPAIR: if (yes) page=CONFIRM; else stop=1; break;
            default: stop=1; break;
            }
            yes=0;
        }
        if (page==ERROR_PAGE) snprintf(err,sizeof err,"%s",problem);
    }
    preview_free(); free(pkg.main.data); free(pkg.additions.data);
    TTF_CloseFont(font); TTF_CloseFont(title); TTF_CloseFont(small);
    return 0;
}

