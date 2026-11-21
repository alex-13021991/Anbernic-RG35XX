#include "ui_fixture.h"
#include "system.h"
#include "photo_report.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static TTF_Font f1=22,f2=18,f3=28;
TTF_Font *font=&f1,*small=&f2,*title=&f3;
const SDL_Color C_TEXT=0xcccccc,C_MUTED=0x96a0aa,C_ACCENT=0x96aabc;
volatile sig_atomic_t stopping;
static FixtureFormat format={16};static FixtureScreen panel={640,480,&format};
FixtureScreen *screen=&panel;
static SDL_Thread job;
static int running,step;
enum { FILES, PHOTOS, LONG_LIST };
static int scenario;
static Uint32 clock_ms=100;
const char *const card_paths[]={"TF1","TF2"};
int ui_init(const char *s) {
    (void)s;
#ifdef __wasi__
    assert(!chdir("/fixtures"));
#endif
    unlink("inventory.cache");
    const char *mode=getenv("SYSINFO_FIXTURE");
    if (mode) {
        scenario=!strcmp(mode,"photos")?PHOTOS:LONG_LIST;
        assert(scenario==PHOTOS || !strcmp(mode,"long-list"));
        Volume v[2];
        static Report sample;
        read_volumes(v);
        photo_report(&sample,v);
        if (scenario==LONG_LIST) {
            /* Populated art directories without a launchable ROM can appear in a full scan. */
            Inventory *tf1=&sample.cards[0];
            tf1->platforms[tf1->nplatforms++]=(Platform){"VECTREX",{0,256}};
            tf1->platforms[tf1->nplatforms++]=(Platform){"ZX",{0,256}};
        }
        assert(cache_save(&sample));
    }
    return 1;
}
void ui_close(void) {unlink("inventory.cache");}
char *SDL_VideoDriverName(char *dst,int n) {snprintf(dst,n,"fixture");return dst;}
const char *SDL_GetError(void) {return "fixture";}
Uint32 SDL_GetTicks(void) {return clock_ms;}
SDL_Thread *SDL_CreateThread(int (*fn)(void *),void *arg) {
    assert(!running);job=(SDL_Thread){fn,arg,0};running=1;return &job;
}
void SDL_WaitThread(SDL_Thread *p,int *rc) {
    assert(p==&job);if (running) {p->fn(p->arg);running=0;}if (rc) *rc=0;
}
void SDL_Delay(Uint32 ms) {
    clock_ms+=ms;
    if (running && ++job.age==2) {job.fn(job.arg);running=0;}
}
int key(void) {
    static const int files[]={KEY_R,KEY_NONE,KEY_NONE,KEY_X,KEY_RIGHT,KEY_RIGHT,KEY_Y,KEY_Y,
        KEY_A,KEY_NONE,KEY_NONE,KEY_A,KEY_B,KEY_NONE,KEY_NONE,KEY_L,KEY_B};
    static const int photos[]={KEY_R,KEY_RIGHT,KEY_RIGHT,KEY_X,KEY_Y,KEY_Y,KEY_RIGHT,KEY_LEFT,
        KEY_LEFT,KEY_L,KEY_R,KEY_RIGHT,KEY_Y,KEY_X,KEY_B};
    static const int scroll[]={KEY_R,KEY_LEFT,KEY_X,KEY_DOWN,KEY_DOWN,KEY_Y,KEY_B};
    const int *keys=scenario==PHOTOS?photos:scenario==LONG_LIST?scroll:files;
    size_t count=(scenario==PHOTOS?sizeof(photos):scenario==LONG_LIST?sizeof(scroll):sizeof(files))/sizeof(keys[0]);
    assert((size_t)step<count);
    return keys[step++];
}
void read_volumes(Volume v[2]) {
    memset(v,0,sizeof(Volume)*2);
    v[0]=(Volume){1,64000000000ULL,28000000000ULL,"fixture1"};
    v[1]=(Volume){1,32000000000ULL,15000000000ULL,"fixture2"};
}
void read_system(System *s) {
    *s=(System){"3.10.0","ARM Cortex-A9","1 ч 12 мин",17,1008,45,72,237568,121856};
}
void box(int x,int y,int w,int h,unsigned rgb) {printf("BOX\t%d\t%d\t%d\t%d\t%06x\n",x,y,w,h,rgb);}
void text(int x,int y,const char *s,TTF_Font *f,SDL_Color c,int w) {
    printf("TEXT\t%d\t%d\t%d\t%d\t%06x\t%s\n",x,y,w,*f,c,s);
}
void center(int x,int y,int w,const char *s,TTF_Font *f,SDL_Color c) {
    printf("CENTER\t%d\t%d\t%d\t%d\t%06x\t%s\n",x,y,w,*f,c,s);
}
void page(const char *h,const char *r) {
    printf("FRAME\nSTEP\t%d\n",step);box(0,0,640,480,0x191f27);box(0,0,640,54,0x485765);
    text(18,12,h,title,C_TEXT,470);text(495,20,r,small,C_TEXT,130);
}
void footer(const char *h) {box(0,445,640,35,0x485765);text(18,452,h,small,C_TEXT,604);}
void flip(void) {puts("FLIP");}

