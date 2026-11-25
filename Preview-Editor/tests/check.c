#include "editor.h"
#include "theme.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

static Pixel canvas[CW*CH];
static void put(const char *path,const Image *im) {
    FILE *f=fopen(path,"wb"); assert(f); assert(write_png(f,im)); assert(!fclose(f));
}
static Rect drawn(void) {
    int l=CW,r=0,t=CH,b=0;
    for (int y=0;y<CH;y++) for (int x=0;x<CW;x++) if (canvas[y*CW+x].a) {
        if (x<l) l=x;
        if (x+1>r) r=x+1;
        if (y<t) t=y;
        if (y+1>b) b=y+1;
    }
    assert(r>l);
    return (Rect){l,t,r-l,b-t};
}
static void geometry(void) {
    Image im={CW,CH,calloc(CW*CH,sizeof(Pixel))};
    Placement p;
    assert(!initial(&im,&p));
    for (int y=90;y<141;y++) for (int x=77;x<207;x++)
        if (x<160 || y>120) im.p[y*CW+x]=(Pixel){63,120,211,x==77 ? 1 : 128};
    assert(initial(&im,&p));
    assert(p.x==77 && p.y==90 && p.crop.w==130 && p.crop.h==51 && p.percent==100);
    trim(&p,&im,0,-7); trim(&p,&im,1,-5);
    assert(p.x==70 && p.y==85 && p.crop.x==70 && p.crop.y==85);
    int scales[]={37,100,173,400};
    int margins[]={0,16,37};
    for (unsigned s=0;s<sizeof scales/sizeof *scales;s++) {
        scale(&p,scales[s]);
        for (unsigned m=0;m<sizeof margins/sizeof *margins;m++) for (int cell=0;cell<9;cell++) {
            int margin=margins[m];
            assert(align_image(&im,&p,cell,margin,43,49));
            assert(p.percent==scales[s]);
            render(&im,p,canvas); Rect b=drawn();
            int col=cell%3,row=cell/3;
            assert(b.x==(col==0 ? margin : col==1 ? (CW-b.w)/2 : CW-margin-b.w));
            assert(b.y==(row==0 ? 43+margin : row==1 ? 43+(CH-43-49-b.h)/2 : CH-49-margin-b.h));
        }
    }
    int before=p.crop.w;
    trim(&p,&im,2,3); assert(p.crop.w==before-3);
    trim(&p,&im,2,-3); assert(p.crop.w==before);
    History h={0}; h.list[0]=p;
    for (int i=0;i<HISTORY+20;i++) { shift(&p,1,0); history_push(&h,p); }
    assert(h.at==HISTORY-1 && h.last==h.at);
    assert(history_move(&h,-1)); assert(history_move(&h,1));
    assert(same_place(h.list[h.at],p)); assert(history_move(&h,-1));
    Placement branch=h.list[h.at]; shift(&branch,0,-4); history_push(&h,branch);
    assert(!history_move(&h,1));
    im.p[0]=(Pixel){220,10,20,255};
    p=(Placement){{0,0,CW,CH},0,0,150};
    assert(align_image(&im,&p,8,16,43,49) && p.percent==150);
    free_image(&im);
    puts("PASS alpha bounds, 108 alignments, crop, zoom, movement and branching history");
}
static void storage(void) {
    Pixel pixels[6]={{10,15,20,0},{20,30,250,1},{200,100,70,254},
                     {100,10,130,128},{20,30,40,9},{255,1,2,255}};
    Image im={3,2,pixels},loaded={0};
    put("art.png",&im);
    Digest first,version,after;
    assert(load_document("art.png",&loaded,first));
    assert(!memcmp(loaded.p,pixels,sizeof pixels)); free_image(&loaded);
    memcpy(version,first,sizeof version);
    Placement p; assert(initial(&im,&p)); render(&im,p,canvas);
    assert(save_document("art.png",canvas,version));
    /* A short write while staging the next PNG must leave both copies intact. */
    struct rlimit saved,limited;
    assert(!getrlimit(RLIMIT_FSIZE,&saved));
    limited=saved; limited.rlim_cur=128;
    signal(SIGXFSZ,SIG_IGN);
    assert(!setrlimit(RLIMIT_FSIZE,&limited));
    assert(!save_document("art.png",canvas,version));
    assert(!setrlimit(RLIMIT_FSIZE,&saved));
    assert(load_document("art.png",&loaded,after) && !memcmp(after,version,sizeof after)); free_image(&loaded);
    assert(has_original("art.png"));
    assert(load_document("art.png",&loaded,after) && !memcmp(after,version,sizeof after));
    assert(loaded.w==CW && loaded.h==CH && !memcmp(loaded.p,canvas,sizeof canvas)); free_image(&loaded);
    shift(&p,12,1); render(&im,p,canvas);
    assert(save_document("art.png",canvas,version));
    int lock=open(".",O_RDONLY|O_DIRECTORY); assert(lock>=0 && !flock(lock,LOCK_EX));
    assert(!save_document("art.png",canvas,version)); close(lock);
    assert(!save_document("art.png",canvas,first));
    assert(restore_document("art.png") && has_original("art.png"));
    assert(load_document("art.png",&loaded,after) && !memcmp(after,first,sizeof after)); free_image(&loaded);
    FILE *f=fopen(".preview-originals/art.png","wb"); assert(f); fputs("broken",f); fclose(f);
    assert(!save_document("art.png",canvas,first)); assert(!restore_document("art.png"));
    assert(load_document("art.png",&loaded,after) && !memcmp(after,first,sizeof after)); free_image(&loaded);
    assert(load_prefs().margin==16);
    assert(save_prefs((Prefs){27,0,-1})); Prefs prefs=load_prefs();
    assert(prefs.margin==27 && prefs.top==0 && prefs.bottom==-1);
    puts("PASS RGBA round trip, repeat save, short write, original bytes, stale writer, lock, bad backup, preferences");
}
static void themes(void) {
    assert(!TTF_Init());
    assert(!mkdir("CFW",0700)); assert(!mkdir("CFW/skin",0700));
    Image overlay={CW,CH,calloc(CW*CH,sizeof(Pixel))};
    for (int y=0;y<CH;y++) for (int x=0;x<CW;x++)
        overlay.p[y*CW+x].a=(y<70 || y>=CH-61) ? 200 : 0;
    put("CFW/skin/game-overlay.png",&overlay);
    Theme t={0};
    theme_load(&t,"."); assert(t.top==70 && t.bottom==61); theme_free(&t);
    FILE *f=fopen("CFW/skin/settings.json","w"); assert(f);
    fputs("{\"top-panel-height\":35,\"bottom-panel-height\":0}",f); fclose(f);
    t=(Theme){0}; theme_load(&t,".");
    assert(t.top==35 && t.bottom==0);
    int top,bottom; theme_panels(&t,(Prefs){16,-1,52},&top,&bottom);
    assert(top==35 && bottom==52);
    theme_free(&t); free_image(&overlay); TTF_Quit();
    puts("PASS overlay measurement, explicit panel heights and manual override");
}
int main(void) {
    char tmp[]="/tmp/preview-check-XXXXXX";
    assert(mkdtemp(tmp) && !chdir(tmp));
    geometry(); storage(); themes();
    const char *files[]={"CFW/skin/settings.json","CFW/skin/game-overlay.png",".preview-originals/art.png","art.png","preview.cfg"};
    for (unsigned i=0;i<sizeof files/sizeof *files;i++) assert(!unlink(files[i]));
    assert(!rmdir(".preview-originals") && !rmdir("CFW/skin") && !rmdir("CFW") && !chdir("/") && !rmdir(tmp));
    return 0;
}
