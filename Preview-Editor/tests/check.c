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
static void rom_exports(void) {
    const char *cases[][2]={
        {"1943 - The Battle of Midway (U) [!]-261120-011053.png","1943 - The Battle of Midway (U) [!].png"},
        {"1943 - The Battle of Midway (U) [!]-261120-011058.png","1943 - The Battle of Midway (U) [!].png"},
        {"AAAHH!!! Real Monsters (UE) [T-Rus uBAH009 (01.02.2019)]-261119-211753.png","AAAHH!!! Real Monsters (UE) [T-Rus uBAH009 (01.02.2019)].png"},
        {"Aladdin (Unl)-261120-011014.png","Aladdin (Unl).png"},
        {"Aladdin Rus-261120-062739.png","Aladdin Rus.png"},
        {"Aladdin Rus-261120-062837.png","Aladdin Rus.png"},
        {"Aladdin Rus-261120-062920.png","Aladdin Rus.png"},
        {"Arkanoid RU (J) [T+Rus Guyver (18.02.2007)]-261120-011046.png","Arkanoid RU (J) [T+Rus Guyver (18.02.2007)].png"},
        {"Asterix (E)-261120-011020.png","Asterix (E).png"},
        {"Batman - Return of the Joker (U) [!]-261120-011027.png","Batman - Return of the Joker (U) [!].png"},
        {"Batman Returns (U) [!]-261121-130345.png","Batman Returns (U) [!].png"},
        {"Batman Returns (U) [!]-261121-130355.png","Batman Returns (U) [!].png"},
        {"Castlevania (E)-261120-232025.png","Castlevania (E).png"},
        {"Contra - Hard Corps RU Hack (U) [!]-261120-210047.png","Contra - Hard Corps RU Hack (U) [!].png"},
        {"Contra - Hard Corps RU Hack (U) [!]-261120-225933.png","Contra - Hard Corps RU Hack (U) [!].png"},
        {"Contra - Hard Corps RU Hack (U) [!]-261120-225955.png","Contra - Hard Corps RU Hack (U) [!].png"},
        {"Galaga (1990)(Namco Ltd.)(J)-261118-114834.png","Galaga (1990)(Namco Ltd.)(J).png"},
        {"Garlic Menu Settings_000.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_001.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_002.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_003.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_004.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_005.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_006.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_007.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_008.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_009.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_010.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_011.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_012.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_013.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_014.png","Garlic Menu Settings.png"},
        {"Garlic Menu Settings_015.png","Garlic Menu Settings.png"},
        {"GarlicOS_000.png","GarlicOS.png"},
        {"GarlicOS_001.png","GarlicOS.png"},
        {"GarlicOS_002.png","GarlicOS.png"},
        {"GarlicOS_003.png","GarlicOS.png"},
        {"GarlicOS_004.png","GarlicOS.png"},
        {"GarlicOS_005.png","GarlicOS.png"},
        {"GarlicOS_006.png","GarlicOS.png"},
        {"GarlicOS_007.png","GarlicOS.png"},
        {"GarlicOS_008.png","GarlicOS.png"},
        {"GarlicOS_009.png","GarlicOS.png"},
        {"GarlicOS_010.png","GarlicOS.png"},
        {"GarlicOS_011.png","GarlicOS.png"},
        {"Jazz Holiday 95_000.png","Jazz Holiday 95.png"},
        {"Jazz Holiday 95_001.png","Jazz Holiday 95.png"},
        {"Jazz Holiday 95_002.png","Jazz Holiday 95.png"},
        {"Jazz Holiday 95_003.png","Jazz Holiday 95.png"},
        {"Jazz Holiday 95_004.png","Jazz Holiday 95.png"},
        {"Jazz Jackrabbit 2_000.png","Jazz Jackrabbit 2.png"},
        {"Jazz Jackrabbit 2_001.png","Jazz Jackrabbit 2.png"},
        {"Jazz Jackrabbit 2_002.png","Jazz Jackrabbit 2.png"},
        {"Jazz Jackrabbit 2_003.png","Jazz Jackrabbit 2.png"},
        {"Jazz Jackrabbit 2_004.png","Jazz Jackrabbit 2.png"},
        {"Jazz Jackrabbit 2_005.png","Jazz Jackrabbit 2.png"},
        {"Jazz Jackrabbit_000.png","Jazz Jackrabbit.png"},
        {"Jazz Jackrabbit_001.png","Jazz Jackrabbit.png"},
        {"Jazz Jackrabbit_002.png","Jazz Jackrabbit.png"},
        {"Jazz Jackrabbit_003.png","Jazz Jackrabbit.png"},
        {"Jazz Jackrabbit_004.png","Jazz Jackrabbit.png"},
        {"Jazz Jackrabbit_005.png","Jazz Jackrabbit.png"},
        {"Jazz Jackrabbit CD_000.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_001.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_002.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_003.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_004.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_005.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_006.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_007.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_008.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_009.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_010.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_011.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_012.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_013.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_014.png","Jazz Jackrabbit CD.png"},
        {"Jazz Jackrabbit CD_015.png","Jazz Jackrabbit CD.png"},
        {"Kero Kero Keroppi no Daibouken 2 - Donuts Ike ha Oosawagi! (J) [T+Eng1.0_Toma]-261120-230409.png","Kero Kero Keroppi no Daibouken 2 - Donuts Ike ha Oosawagi! (J) [T+Eng1.0_Toma].png"},
        {"retroarch_000.png","retroarch.png"},
        {"Screenshot Manager_000.png","Screenshot Manager.png"},
        {"Screenshot Viewer_000.png","Screenshot Viewer.png"},
        {"Screenshot Viewer_001.png","Screenshot Viewer.png"},
        {"Super Mario Bros-261120-010927.png","Super Mario Bros.png"},
        {"Super Mario Bros-261120-010943.png","Super Mario Bros.png"},
        {"USB Manager_000.png","USB Manager.png"},
        {"Jazz Jackrabbit 2.PNG","Jazz Jackrabbit 2.PNG"},
        {"Game_12.png","Game_12.png"},
        {"Game_1234.png","Game_1234.png"},
        {"Game_abc.png","Game_abc.png"},
        {"Game-261121-13035.png","Game-261121-13035.png"},
        {"Game_001-261121-130355.png","Game_001.png"},
        {"_001.png","_001.png"}
    };
    char name[256],target[4096];
    for (unsigned i=0;i<sizeof cases/sizeof *cases;i++) {
        assert(rom_filename(cases[i][0],name,sizeof name));
        assert(!strcmp(name,cases[i][1]));
    }
    assert(!rom_filename("Game_001.png",name,4));
    assert(!mkdir("card1",0700) && !mkdir("card2",0700));
    Pixel pixels[1]={{1,2,3,255}}; Image im={1,1,pixels},loaded={0};
    const char *source="card1/Game_002.PNG";
    put(source,&im);
    Digest version,after; assert(load_document(source,&loaded,version)); free_image(&loaded);
    assert(export_rom(source,"card1",NULL,version,0,target,sizeof target)==1);
    assert(!strcmp(target,"card1/Img_Gen/Game.PNG"));
    assert(load_document(target,&loaded,after) && !memcmp(version,after,sizeof after)); free_image(&loaded);
    assert(export_rom(source,"card1",NULL,version,0,target,sizeof target)==2);
    assert(export_rom(source,"card1",NULL,version,1,target,sizeof target)==1);
    assert(!export_rom("card1/Img_Gen/Game.PNG","card1",NULL,version,1,target,sizeof target));
    Placement place; assert(initial(&im,&place)); render(&im,place,canvas);
    assert(export_rom(source,"card2",canvas,version,0,target,sizeof target)==1);
    assert(load_document(target,&loaded,after) && loaded.w==CW && loaded.h==CH &&
           !memcmp(canvas,loaded.p,sizeof canvas)); free_image(&loaded);
    assert(load_document(source,&loaded,after) && !memcmp(version,after,sizeof after)); free_image(&loaded);
    assert(!unlink("card2/Img_Gen/Game.PNG"));
    assert(!symlink("../../card1/Game_002.PNG","card2/Img_Gen/Game.PNG"));
    assert(!export_rom(source,"card2",NULL,version,1,target,sizeof target));
    assert(!unlink("card2/Img_Gen/Game.PNG"));
    pixels[0].r++; put(source,&im);
    assert(!export_rom(source,"card2",NULL,version,0,target,sizeof target));
    assert(access("card2/Img_Gen/Game.PNG",F_OK));
    assert(!unlink("card1/Img_Gen/Game.PNG") && !unlink(source));
    assert(!rmdir("card1/Img_Gen") && !rmdir("card2/Img_Gen"));
    assert(!rmdir("card1") && !rmdir("card2"));
    puts("PASS ROM names from complete supplied file list, extension case, exact copy, canvas export, collision, same source, stale source and symlink");
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
    geometry(); storage(); rom_exports(); themes();
    const char *files[]={"CFW/skin/settings.json","CFW/skin/game-overlay.png",".preview-originals/art.png","art.png","preview.cfg"};
    for (unsigned i=0;i<sizeof files/sizeof *files;i++) assert(!unlink(files[i]));
    assert(!rmdir(".preview-originals") && !rmdir("CFW/skin") && !rmdir("CFW") && !chdir("/") && !rmdir(tmp));
    return 0;
}
