#include "ui.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

SDL_Surface *screen;
TTF_Font *title_font,*font,*small;
Pixel canvas[CW*CH];
static SDL_Surface *scene,*mini;
static volatile sig_atomic_t stopping;
static void halt(int sig) { (void)sig; stopping=1; }
int ui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)<0 || TTF_Init()<0) return 0;
    screen=SDL_SetVideoMode(CW,CH,16,SDL_SWSURFACE);
    scene=SDL_CreateRGBSurface(SDL_SWSURFACE,CW,CH,16,0xf800,0x07e0,0x001f,0);
    mini=SDL_CreateRGBSurface(SDL_SWSURFACE,512,384,16,0xf800,0x07e0,0x001f,0);
    title_font=TTF_OpenFont("assets/font.ttf",25);
    font=TTF_OpenFont("assets/font.ttf",21);
    small=TTF_OpenFont("assets/font.ttf",17);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(340,65);
    signal(SIGINT,halt); signal(SIGTERM,halt);
    return screen && scene && mini && title_font && font && small;
}
void ui_close(void) {
    if (title_font) TTF_CloseFont(title_font);
    if (font) TTF_CloseFont(font);
    if (small) TTF_CloseFont(small);
    SDL_FreeSurface(scene); SDL_FreeSurface(mini);
    TTF_Quit(); SDL_Quit();
}
Key read_key(void) {
    static unsigned char held[SDLK_LAST];
    SDL_Event e;
    while (!stopping) {
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_QUIT) { stopping=1; return QUIT; }
            if (e.type!=SDL_KEYDOWN && e.type!=SDL_KEYUP) continue;
            unsigned sym=e.key.keysym.sym;
            if (sym>=SDLK_LAST) continue;
            if (e.type==SDL_KEYUP) { held[sym]=0; continue; }
            Key k=NONE;
            switch (sym) {
            case SDLK_w: case SDLK_UP: k=UP; break;
            case SDLK_s: case SDLK_DOWN: k=DOWN; break;
            case SDLK_q: case SDLK_LEFT: k=LEFT; break;
            case SDLK_d: case SDLK_RIGHT: k=RIGHT; break;
            case SDLK_a: case SDLK_RETURN: k=A; break;
            case SDLK_b: case SDLK_ESCAPE: k=B; break;
            case SDLK_x: k=X; break;
            case SDLK_y: k=Y; break;
            case SDLK_h: case SDLK_j: case SDLK_PAGEUP: k=L; break;
            case SDLK_l: case SDLK_k: case SDLK_PAGEDOWN: k=R; break;
            case SDLK_u: k=MENU; break;
            }
            if (held[sym] && !(k==UP || k==DOWN || k==LEFT || k==RIGHT || k==L || k==R)) continue;
            held[sym]=1;
            if (k!=NONE) return k;
        }
        SDL_Delay(15);
    }
    return QUIT;
}
static void rect(SDL_Surface *dst,int x,int y,int w,int h,int color) {
    if (w<=0 || h<=0) return;
    SDL_Rect r={(Sint16)x,(Sint16)y,(Uint16)w,(Uint16)h};
    SDL_FillRect(dst,&r,SDL_MapRGB(dst->format,color>>16,color>>8,color));
}
void box(int x,int y,int w,int h,int color) { rect(screen,x,y,w,h,color); }
void label(int x,int y,int width,TTF_Font *f,int color,const char *s) {
    if (!s || !*s || width<=0) return;
    SDL_Color fg={(Uint8)(color>>16),(Uint8)(color>>8),(Uint8)color,0};
    SDL_Surface *text=TTF_RenderUTF8_Blended(f,s,fg);
    if (!text) return;
    SDL_Rect src={0,0,(Uint16)clamp(width,0,text->w),(Uint16)text->h};
    SDL_Rect dst={(Sint16)x,(Sint16)y,0,0};
    SDL_BlitSurface(text,&src,screen,&dst);
    SDL_FreeSurface(text);
}
void header(const char *s,const char *right) {
    box(0,0,CW,CH,BG); box(0,44,CW,1,LINE);
    int w=0,h=0;
    if (right && *right) {
        TTF_SizeUTF8(small,right,&w,&h);
        label(CW-w-16,15,w,small,ACCENT,right);
    }
    label(16,7,CW-42-w,title_font,INK,s);
}
void footer(const char *first,const char *second) {
    box(0,436,CW,44,PANEL);
    label(15,438,610,small,INK,first);
    label(15,458,610,small,MUTED,second);
}
void list_row(int i,int selected,const char *left,const char *right) {
    int y=82+i*38;
    int w=0,h=0;
    if (selected) { box(12,y,616,35,SELECTED); box(12,y,3,35,ACCENT); }
    if (right && *right) {
        TTF_SizeUTF8(small,right,&w,&h);
        label(608-w,y+8,w,small,MUTED,right);
    }
    label(25,y+4,578-w,font,INK,left);
}
static Pixel blend(Pixel under,Pixel over) {
    if (over.a==255) return over;
    if (!over.a) return under;
    unsigned a=over.a,inv=255-a;
    return (Pixel){(over.r*a+under.r*inv+127)/255,(over.g*a+under.g*inv+127)/255,
        (over.b*a+under.b*inv+127)/255,255};
}
static void guide_rect(int x,int y,int w,int h,int color) {
    if (w<1 || h<1) return;
    int right=x+w-1,bottom=y+h-1;
    for (int a=clamp(x,0,CW);a<=right && a<CW;a+=8) {
        int n=clamp(right-a+1,1,4);
        if (y>=0 && y<CH) rect(scene,a,y,n,1,color);
        if (bottom>=0 && bottom<CH) rect(scene,a,bottom,n,1,color);
    }
    for (int a=clamp(y,0,CH);a<=bottom && a<CH;a+=8) {
        int n=clamp(bottom-a+1,1,4);
        if (x>=0 && x<CW) rect(scene,x,a,1,n,color);
        if (right>=0 && right<CW) rect(scene,right,a,1,n,color);
    }
}
static const char *last_part(const char *path) {
    const char *s=strrchr(path,'/');
    return s ? s+1 : path;
}
static int skin_icon(const Image *im,int x,int y,int width,int height) {
    if (!im->p) return 0;
    Placement p={{0,0,im->w,im->h},0,0,100};
    Rect b;
    if (!visible_bounds(im,p,&b)) return 1;
    int w=width,h=width*b.h/b.w;
    if (h>height) { h=height; w=height*b.w/b.h; }
    w=clamp(w,1,width); h=clamp(h,1,height);
    x+=(width-w)/2; y+=(height-h)/2;
    for (int j=0;j<h;j++) {
        Uint16 *row=(Uint16 *)((char *)scene->pixels+(y+j)*scene->pitch);
        const Pixel *src=im->p+(b.y+j*b.h/h)*im->w+b.x;
        for (int i=0;i<w;i++) {
            Uint16 old=row[x+i];
            Pixel under={(old>>11)*255/31,((old>>5)&63)*255/63,(old&31)*255/31,255};
            Pixel c=blend(under,src[i*b.w/w]);
            row[x+i]=(Uint16)((c.r>>3)<<11 | (c.g>>2)<<5 | c.b>>3);
        }
    }
    return 1;
}
static const int button_x[]={18,206,348,468};
static const char *button_letters[]={"","A","B","Y"};
static void menu_icons(const Theme *t) {
    int c=t->guide;
    rect(scene,27,14,20,28,c); rect(scene,30,17,14,13,BG);
    rect(scene,31,35,6,2,BG); rect(scene,33,33,2,6,BG); rect(scene,40,34,3,3,BG);
    if (!skin_icon(&t->battery,579,14,44,28)) {
        rect(scene,579,16,40,24,c); rect(scene,582,19,34,18,BG);
        rect(scene,585,22,27,12,ACCENT); rect(scene,619,22,4,12,c);
    }
    for (int i=0;i<4;i++) {
        int x=button_x[i];
        if (skin_icon(&t->buttons[i],x,431,28,29)) continue;
        if (!i) {
            rect(scene,x+9,431,10,29,c); rect(scene,x,440,28,10,c);
            rect(scene,x+12,443,4,4,BG);
        } else {
            for (int dy=-13;dy<=13;dy++) {
                int dx=13;
                while (dx*dx+dy*dy>13*13) dx--;
                rect(scene,x+14-dx,446+dy,dx*2+1,1,c);
            }
        }
    }
}
static void menu_text(const Theme *t,const Browser *b) {
    char parent[PATH_MAX],name[NAME_MAX+1],clock_text[16]="";
    strcpy(parent,b->folder);
    if (!strcasecmp(last_part(parent),"Imgs")) {
        char *slash=strrchr(parent,'/');
        if (slash) *slash=0;
    }
    time_t now=time(NULL);
    struct tm *tm=localtime(&now);
    if (tm) strftime(clock_text,sizeof clock_text,"%H:%M",tm);
    label(64,12,415,title_font,t->guide,last_part(parent));
    label(500,12,78,title_font,t->guide,clock_text);
    TTF_Font *f=t->list_font ? t->list_font : font;
    int height=TTF_FontHeight(f);
    int first=b->row;
    while (first>0 && !b->entries[first-1].folder) first--;
    int start=clamp(b->row-3,first,b->count-first>8 ? b->count-8 : first);
    int count=clamp(b->count-start,0,8);
    for (int i=0;i<count;i++) {
        const Entry *entry=&b->entries[start+i];
        snprintf(name,sizeof name,"%s",entry->name);
        char *ext=strrchr(name,'.');
        if (ext) *ext=0;
        int w=0,h=0;
        if (TTF_SizeUTF8(f,name,&w,&h)) continue;
        int x=t->alignment==1 ? (CW-w)/2 : t->alignment==2 ? CW-w-t->margin : t->margin;
        int width=!t->alignment && t->width ? t->width : CW+clamp(-x,0,8192);
        label(x,(CH-8*height)/2+i*height,width,f,start+i==b->row ? t->active : t->inactive,name);
    }
    const char *names[]={"Перемещение","Открыть","Назад","В избранное"};
    for (int i=0;i<4;i++) {
        if (i && !t->buttons[i].p) label(button_x[i]+7,433,20,font,BG,button_letters[i]);
        if (t->labels) {
            TTF_Font *guide=t->guide_font ? t->guide_font : small;
            int x=button_x[i]+37,end=i==3 ? CW-4 : button_x[i+1]-3;
            label(x,446-TTF_FontHeight(guide)/2,end-x,guide,t->guide,names[i]);
        }
    }
}
void draw_art(const Image *img,Placement p,const Theme *t,Prefs prefs,int full,const Browser *b) {
    render(img,p,canvas);
    for (int y=0;y<CH;y++) {
        Uint16 *row=(Uint16 *)((char *)scene->pixels+y*scene->pitch);
        for (int x=0;x<CW;x++) {
            int i=y*CW+x;
            int gray=((x/12+y/12)&1) ? 72 : 53;
            Pixel bg=full ? (Pixel){32,44,41,255} : (Pixel){gray,gray+3,gray+3,255};
            if (full && t->background.p) bg=blend(bg,t->background.p[i]);
            Pixel result=blend(bg,canvas[i]);
            if (full && t->overlay.p) result=blend(result,t->overlay.p[i]);
            row[x]=(Uint16)((result.r>>3)<<11 | (result.g>>2)<<5 | result.b>>3);
        }
    }
    if (full) {
        menu_icons(t);
        SDL_BlitSurface(scene,NULL,screen,NULL);
        menu_text(t,b);
    } else {
        int top,bottom;
        theme_panels(t,prefs,&top,&bottom);
        guide_rect(prefs.margin,top+prefs.margin,CW-2*prefs.margin,CH-top-bottom-2*prefs.margin,0x8fbdad);
        guide_rect(p.x,p.y,output_size(p.crop.w,p.percent),output_size(p.crop.h,p.percent),0xffce79);
        SDL_Rect pos={64,49,0,0};
        SDL_SoftStretch(scene,NULL,mini,NULL); SDL_BlitSurface(mini,NULL,screen,&pos);
    }
}
