#include "theme.h"
#include "yyjson.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void skin_image(const char *card,const char *name,Image *im,int fullscreen) {
    char path[PATH_MAX];
    if (snprintf(path,sizeof path,"%s/CFW/skin/%s.png",card,name)>=(int)sizeof path) return;
    FILE *f=fopen(path,"rb");
    if (f) {
        if (read_png(f,im) && (fullscreen ? im->w!=CW || im->h!=CH : im->w>160 || im->h>160)) free_image(im);
        fclose(f);
    }
}
/* Only a horizontal band covering most of the row is evidence of a panel.
   Ignore vertical gradients, side decoration and an all-over tint. */
static int edge_band(const Image *im,int bottom) {
    if (!im->p) return 0;
    for (int n=0;n<=120;n++) {
        const Pixel *row=im->p+(bottom ? CH-1-n : n)*CW;
        int count=0;
        for (int x=0;x<CW;x++) count+=row[x].a>=16;
        if (count<CW*2/3) return n;
    }
    return 0;
}
static yyjson_doc *get_json(const char *file) {
    FILE *f=fopen(file,"rb");
    if (!f) return NULL;
    yyjson_doc *doc=NULL;
    if (!fseek(f,0,SEEK_END)) {
        long size=ftell(f);
        if (size>0 && size<=512*1024 && !fseek(f,0,SEEK_SET)) {
            char *data=malloc(size);
            if (data && fread(data,1,size,f)==(size_t)size) doc=yyjson_read(data,size,0);
            free(data);
        }
    }
    fclose(f);
    return doc;
}
static int number(yyjson_val *obj,const char *key,int fallback,int lo,int hi) {
    yyjson_val *v=yyjson_obj_get(obj,key);
    int64_t n=yyjson_get_sint(v);
    return yyjson_is_int(v) && n>=lo && n<=hi ? (int)n : fallback;
}
static int color(yyjson_val *obj,const char *key,int fallback) {
    const char *s=yyjson_get_str(yyjson_obj_get(obj,key));
    if (!s || *s!='#' || strlen(s)!=7 || strspn(s+1,"abcdefABCDEF0123456789")!=6) return fallback;
    return (int)strtol(s+1,NULL,16);
}
static int simple_name(const char *s) {
    return s && *s && !strchr(s,'/') && !strchr(s,'\\') && strcmp(s,".") && strcmp(s,"..");
}
static int font_size(yyjson_val *o,const char *name,int fallback) {
    const char *s=yyjson_get_str(yyjson_obj_get(o,name));
    if (s && strlen(s)<=2 && *s && strspn(s,"0123456789")==strlen(s))
        return clamp(atoi(s),8,72);
    return number(o,name,fallback,8,72);
}
static void menu_fonts(Theme *t,const char *card) {
    char path[PATH_MAX],lang[160]="English";
    snprintf(path,sizeof path,"%s/CFW/language.flag",card);
    FILE *f=fopen(path,"r");
    if (f) {
        if (fgets(lang,sizeof lang,f)) lang[strcspn(lang,"\r\n")]=0;
        fclose(f);
    }
    if (!simple_name(lang)) strcpy(lang,"English");
    snprintf(path,sizeof path,"%s/CFW/lang/%s.json",card,lang);
    yyjson_doc *d=get_json(path);
    yyjson_val *o=yyjson_doc_get_root(d);
    const char *name=yyjson_get_str(yyjson_obj_get(o,"font"));
    int size=font_size(o,"font-size",28),guide=font_size(o,"button-guide-font-size",19);
    if (snprintf(path,sizeof path,"%s/CFW/font/%s",card,simple_name(name)?name:"Oswald-Regular.ttf")<(int)sizeof path) {
        t->list_font=TTF_OpenFont(path,size);
        t->guide_font=TTF_OpenFont(path,guide);
    }
    if (!t->list_font) t->list_font=TTF_OpenFont("assets/font.ttf",size);
    if (!t->guide_font) t->guide_font=TTF_OpenFont("assets/font.ttf",guide);
    yyjson_doc_free(d);
}
void theme_load(Theme *t,const char *tf1) {
    char path[PATH_MAX];
    skin_image(tf1,"background",&t->background,1);
    skin_image(tf1,"game-overlay",&t->overlay,1);
    const char *icons[]={"icon-dpad-54","icon-A-54","icon-B-54","icon-Y-54"};
    for (int i=0;i<4;i++) skin_image(tf1,icons[i],&t->buttons[i],0);
    skin_image(tf1,"power-full-icon",&t->battery,0);
    snprintf(path,sizeof path,"%s/CFW/skin/settings.json",tf1);
    yyjson_doc *doc=get_json(path);
    yyjson_val *o=yyjson_doc_get_root(doc);
    /* Visible rows in GarlicOS_011.png are 14..42 and 431..459 (inclusive). */
    t->top=number(o,"top-panel-height",clamp(edge_band(&t->overlay,0),43,120),0,120);
    t->bottom=number(o,"bottom-panel-height",clamp(edge_band(&t->overlay,1),49,120),0,120);
    t->margin=number(o,"text-margin",16,0,CW);
    t->width=number(o,"text-max-width",0,0,CW);
    t->active=color(o,"color-active",0x55bef4);
    t->inactive=color(o,"color-inactive",0x909b99);
    t->guide=color(o,"color-guide",0xffffff);
    t->labels=!yyjson_is_false(yyjson_obj_get(o,"guide-button-text-visibility"));
    const char *a=yyjson_get_str(yyjson_obj_get(o,"text-alignment"));
    t->alignment=a && !strcasecmp(a,"right") ? 2 : a && !strcasecmp(a,"center") ? 1 : 0;
    yyjson_doc_free(doc);
    menu_fonts(t,tf1);
}
void theme_free(Theme *t) {
    free_image(&t->background); free_image(&t->overlay);
    for (int i=0;i<4;i++) free_image(&t->buttons[i]);
    free_image(&t->battery);
    if (t->list_font) TTF_CloseFont(t->list_font);
    if (t->guide_font) TTF_CloseFont(t->guide_font);
}
void theme_panels(const Theme *t,Prefs p,int *top,int *bottom) {
    *top=p.top<0 ? t->top : p.top;
    *bottom=p.bottom<0 ? t->bottom : p.bottom;
}
