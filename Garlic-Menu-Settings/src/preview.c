#include "preview.h"
#include <SDL_image.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Surface *canvas,*background,*art,*rows[8];
static const char *games[]={"Battle City","Castlevania","Contra",
    "Northern Passage — Beyond the Mountains","Adventure Island III","Metroid","Star Force","Tank 1990"};
static int color(yyjson_val *obj,const char *key,int fallback) {
    const char *s=yyjson_get_str(yyjson_obj_get(obj,key));
    if (!s || strlen(s)!=7 || *s!='#' || strspn(s+1,"0123456789abcdefABCDEF")!=6) return fallback;
    return (int)strtol(s+1,NULL,16);
}
static yyjson_doc *optional_json(const char *path) {
    Blob b={0};
    if (!read_blob(path,&b)) return NULL;
    yyjson_doc *d=json_object(&b); free(b.data); return d;
}
static int basename_ok(const char *s) {
    return s && *s && strlen(s)<150 && !strchr(s,'/') && !strchr(s,'\\') && strcmp(s,".") && strcmp(s,"..");
}
int preview_init(SDL_Surface *video,const char *assets) {
    char path[PATH_MAX],lang[160]="English";
    Blob flag={0};
    if (read_blob("language.flag",&flag)) {
        flag.data[strcspn(flag.data,"\r\n")]=0;
        if (basename_ok(flag.data)) strcpy(lang,flag.data);
        free(flag.data);
    }
    snprintf(path,sizeof path,"lang/%s.json",lang);
    yyjson_doc *d=optional_json(path);
    if (!d && strcmp(lang,"English")) d=optional_json("lang/English.json");
    yyjson_val *language=yyjson_doc_get_root(d);
    yyjson_val *f=yyjson_obj_get(language,"font-size");
    const char *size=yyjson_is_raw(f) ? yyjson_get_raw(f) : yyjson_get_str(f);
    int pt=size ? atoi(size) : 28;
    if (pt<1 || pt>128) pt=28;
    const char *name=yyjson_get_str(yyjson_obj_get(language,"font"));
    snprintf(path,sizeof path,"font/%s",basename_ok(name) ? name : "Oswald-Regular.ttf");
    TTF_Font *font=TTF_OpenFont(path,pt);
    yyjson_doc_free(d);
    if (!font) {
        snprintf(path,sizeof path,"%s/font.ttf",assets);
        font=TTF_OpenFont(path,pt);
    }
    if (!font) return fail("Шрифт предпросмотра недоступен.");
    SDL_PixelFormat *fmt=video->format;
    canvas=SDL_CreateRGBSurface(SDL_SWSURFACE,640,480,fmt->BitsPerPixel,fmt->Rmask,fmt->Gmask,fmt->Bmask,0);
    snprintf(path,sizeof path,"%s/preview.png",assets);
    art=IMG_Load(path);
    background=IMG_Load("skin/background.png");
    d=optional_json(CFG);
    int active=color(yyjson_doc_get_root(d),"color-active",0xffffff);
    int idle=color(yyjson_doc_get_root(d),"color-inactive",0x839589);
    yyjson_doc_free(d);
    int ok=canvas && art;
    for (int i=0;i<8;i++) {
        int c=i==3 ? active : idle;
        SDL_Color ink={(Uint8)(c>>16),(Uint8)(c>>8),(Uint8)c,0};
        rows[i]=TTF_RenderUTF8_Blended(font,games[i],ink);
        if (!rows[i]) ok=0;
    }
    TTF_CloseFont(font);
    return ok || fail("Не удалось загрузить изображение или строки предпросмотра.");
}
void preview_draw(SDL_Surface *video,Settings s,int cover,int right,int full,unsigned *start) {
    static unsigned previous;
    unsigned now=SDL_GetTicks();
    if (now-previous>2048) *start=now;
    previous=now;
    Row selected=layout(s,cover,rows[3]->w,1,0);
    if (s.scroll && selected.width>0 && selected.width<rows[3]->w) {
        unsigned travel=((rows[3]->w-selected.width)*1000u+s.speed-1)/s.speed;
        if (now-*start>=s.delay+travel+s.pause) *start=now;
    }
    unsigned elapsed=now-*start;
    SDL_FillRect(canvas,NULL,SDL_MapRGB(canvas->format,26,35,33));
    if (background) SDL_BlitSurface(background,NULL,canvas,NULL);
    if (cover) {
        SDL_Rect to={(Sint16)(right ? 640-art->w-10 : 10),(Sint16)((480-art->h)/2),0,0};
        SDL_BlitSurface(art,NULL,canvas,&to);
    }
    int h=rows[0]->h;
    for (int i=0;i<8;i++) {
        Row r=layout(s,cover,rows[i]->w,i==3,elapsed);
        if (r.width<=0) continue;
        SDL_Rect src={(Sint16)r.offset,0,(Uint16)r.width,(Uint16)rows[i]->h};
        SDL_Rect dst={(Sint16)r.x,(Sint16)((480-h*8)/2+i*h),0,0};
        SDL_BlitSurface(rows[i],&src,canvas,&dst);
    }
    if (full) SDL_BlitSurface(canvas,NULL,video,NULL);
    else {
        SDL_Rect frame={318,99,304,228};
        SDL_SoftStretch(canvas,NULL,video,&frame);
    }
}
void preview_free(void) {
    SDL_FreeSurface(canvas); SDL_FreeSurface(background); SDL_FreeSurface(art);
    for (int i=0;i<8;i++) SDL_FreeSurface(rows[i]);
}

