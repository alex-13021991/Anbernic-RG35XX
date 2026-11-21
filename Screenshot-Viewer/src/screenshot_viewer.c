#include "ui.h"
#include "core.h"
#include "gallery.h"
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
static Gallery gallery;
static int selected,full,original,panx,pany,cached_page=-1,current_index=-1;
static SDL_Surface *thumbs[6],*current;
static void clear_images(void) {
    for(int i=0;i<6;i++){if(thumbs[i]) SDL_FreeSurface(thumbs[i]);thumbs[i]=NULL;}
    if(current) SDL_FreeSurface(current);current=NULL;cached_page=current_index=-1;
}
static SDL_Surface *load_image(const char *path) {
    int w,h; if(!png_dimensions(path,&w,&h)) return NULL;
    SDL_Surface *src=IMG_Load(path); if(!src) {fprintf(stderr,"PNG %s: %s\n",path,IMG_GetError());return NULL;}
    /* Flatten transparency on the application's background before scaling. */
    SDL_Surface *dest=SDL_CreateRGBSurface(SDL_SWSURFACE,src->w,src->h,16,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
    if(dest) {SDL_FillRect(dest,NULL,SDL_MapRGB(dest->format,25,20,34));SDL_BlitSurface(src,NULL,dest,NULL);}
    SDL_FreeSurface(src);return dest;
}
static void cache_page(void) {
    int p=selected/6;if(p==cached_page)return;
    clear_images(); cached_page=p;
    for(int i=0;i<6 && (size_t)(p*6+i)<gallery.count;i++) {
        SDL_Surface *src=load_image(gallery.shots[p*6+i].path); if(!src) continue;
        int w=src->w,h=src->h; double sc=186.0/w; if(122.0/h<sc)sc=122.0/h;
        w=(int)(w*sc);h=(int)(h*sc);if(w<1)w=1;if(h<1)h=1;
        thumbs[i]=SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,16,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
        if(thumbs[i]) {SDL_Rect a={0,0,(Uint16)src->w,(Uint16)src->h},b={0,0,(Uint16)w,(Uint16)h};SDL_SoftStretch(src,&a,thumbs[i],&b);}
        SDL_FreeSurface(src);
    }
}
static void render(void) {
    char info[160];snprintf(info,sizeof(info),"%u файлов",(unsigned)gallery.count);
    page("СКРИНШОТЫ",info);
    if(!gallery.count) {
        center(20,170,600,"Скриншоты не найдены",title,C_TEXT);
        center(20,219,600,"PNG из папок Screenshots на TF1 и TF2",small,C_MUTED);
        center(20,252,600,"Дополнительные папки: screenshots.cfg",small,C_MUTED);
        footer("Y Обновить   B Выход");flip();return;
    }
    if(full) {
        if(current_index!=selected) {
            if(current) SDL_FreeSurface(current);current=load_image(gallery.shots[selected].path);current_index=selected;
        }
        box(0,0,640,445,0x191422);
        if(current) {
            if(original) {
                int maxx=current->w>640?current->w-640:0,maxy=current->h>445?current->h-445:0;
                if(panx<0)panx=0;if(pany<0)pany=0;if(panx>maxx)panx=maxx;if(pany>maxy)pany=maxy;
                int w=current->w<640?current->w:640,h=current->h<445?current->h:445;
                SDL_Rect src={(Sint16)panx,(Sint16)pany,(Uint16)w,(Uint16)h},dst={(Sint16)((640-w)/2),(Sint16)((445-h)/2),0,0};
                SDL_BlitSurface(current,&src,screen,&dst);
            } else show_image(current,0,0,640,445);
        } else center(20,195,600,"PNG повреждён или слишком большой",font,C_MUTED);
        footer(original?"↑↓←→ Сдвиг   L/R Листать   Y Вписать   X Удалить   B Назад":"←→ Листать   Y Размер 1:1   X Удалить   B Назад");
        flip();return;
    }
    cache_page();
    for(int i=0;i<6;i++) {
        int index=cached_page*6+i;if((size_t)index>=gallery.count)break;
        int x=18+(i%3)*204,y=67+(i/3)*158;
        box(x,y,196,132,0x0d0b12);
        if(thumbs[i]) show_image(thumbs[i],x+5,y+5,186,122);
        else center(x+5,y+51,186,"Нет превью",small,C_MUTED);
        if(selected==index) outline(x-2,y-2,200,136,0xb992ef);
        box(x+6,y+6,38,23,0x30253f);snprintf(info,sizeof(info),"TF%d",gallery.shots[index].card);
        text(x+9,y+7,info,small,C_TEXT,34);
        text(x,y+135,shot_name(&gallery.shots[index]),small,selected==index?C_ACCENT:C_TEXT,196);
    }
    box(18,389,604,1,0x655279);
    snprintf(info,sizeof(info),"TF%d · %d / %u",gallery.shots[selected].card,selected+1,(unsigned)gallery.count);
    text(18,397,info,small,C_ACCENT,150);text(172,397,shot_name(&gallery.shots[selected]),small,C_TEXT,450);
    if(gallery.limited) text(18,422,"Достигнут лимит сканирования; часть файлов не показана.",small,C_RED,604);
    else if(gallery.errors) text(18,422,"Часть папок недоступна. Подробности: app.log",small,C_RED,604);
    footer("A Открыть   X Удалить   Y Обновить   L/R Страница   B Выход");flip();
}
static void rescan(void) {
    page("СКРИНШОТЫ",NULL);center(20,213,600,"Поиск на TF1 и TF2...",font,C_TEXT);flip();
    clear_images();gallery_free(&gallery);
    if(!gallery_scan(&gallery,"/mnt/mmc","/mnt/SDCARD","screenshots.cfg")) message("Недостаточно памяти","Не удалось открыть галерею.","Закройте приложение и повторите.");
    fprintf(stderr,"Scan: %u PNG; errors=%d, limited=%d\n",(unsigned)gallery.count,gallery.errors,gallery.limited);
    if(selected>=(int)gallery.count)selected=gallery.count?(int)gallery.count-1:0;
    if(!gallery.count) full=0;
}
static void remove_selected(void) {
    if(!gallery.count)return;
    char info[128];snprintf(info,sizeof(info),"Карта TF%d. Удаление безвозвратно.",gallery.shots[selected].card);
    if(!confirm("Удалить скриншот?",shot_name(&gallery.shots[selected]),info,"Удалить"))return;
    if(gallery_delete(&gallery,(size_t)selected)<0) {
        fprintf(stderr,"Delete failed: %s\n",strerror(errno));
        message("Не удалось удалить","Файл изменён, недоступен или карта защищена.","Обновите список кнопкой Y.");return;
    }
    sync();clear_images();
    if(selected>=(int)gallery.count)selected=gallery.count?(int)gallery.count-1:0;
    if(!gallery.count)full=0;
    panx=pany=0;
}
int main(void) {
    if(!ui_init("Screenshot Viewer"))return 1;
    if(!(IMG_Init(IMG_INIT_PNG)&IMG_INIT_PNG)) {fprintf(stderr,"PNG support: %s\n",IMG_GetError());ui_close();return 1;}
    rescan();int done=0,dirty=1;
    while(!done && !stopping) {
        if(dirty){render();dirty=0;}int k=key(),before=selected;if(k!=KEY_NONE)dirty=1;
        if(k==KEY_QUIT)done=1;
        if(k==KEY_B || k==KEY_MENU) {if(full){full=0;original=0;}else done=1;}
        if(k==KEY_Y) {if(full){original=!original;panx=pany=0;}else rescan();}
        if(gallery.count) {
            if(k==KEY_X)remove_selected();
            else if(k==KEY_A && !full){full=1;original=0;}
            else if(full) {
                if(original) {
                    if(k==KEY_LEFT)panx-=32;if(k==KEY_RIGHT)panx+=32;
                    if(k==KEY_UP)pany-=32;if(k==KEY_DOWN)pany+=32;
                }
                if(k==KEY_L || (!original && k==KEY_LEFT)) selected=selected?selected-1:(int)gallery.count-1;
                if(k==KEY_R || (!original && k==KEY_RIGHT)) selected=(selected+1)%(int)gallery.count;
            } else {
                if(k==KEY_LEFT)selected=grid_move(selected,(int)gallery.count,-1,0);
                if(k==KEY_RIGHT)selected=grid_move(selected,(int)gallery.count,1,0);
                if(k==KEY_UP)selected=grid_move(selected,(int)gallery.count,0,-1);
                if(k==KEY_DOWN)selected=grid_move(selected,(int)gallery.count,0,1);
                if(k==KEY_L) {selected-=6;if(selected<0)selected=0;}
                if(k==KEY_R) {selected+=6;if(selected>=(int)gallery.count)selected=(int)gallery.count-1;}
            }
        }
        if(before!=selected)panx=pany=0;
        SDL_Delay(25);
    }
    clear_images();gallery_free(&gallery);IMG_Quit();ui_close();return 0;
}
