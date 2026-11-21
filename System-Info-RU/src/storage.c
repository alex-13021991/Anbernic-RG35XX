#include "storage.h"
#include "diag.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
enum { MAX_DEPTH = 48 };
void format_bytes(char *dst,size_t cap,uint64_t n) {
    const char *units[]={"Б","КиБ","МиБ","ГиБ","ТиБ"};
    int i=0;
    double value=(double)n;
    while (value>=1024.0 && i<4) { value/=1024.0; i++; }
    snprintf(dst,cap,i?"%.1f %s":"%.0f %s",value,units[i]);
    char *point=strchr(dst,'.'); if (point) *point=',';
}
typedef struct { char *path; int referenced; } Game;
typedef struct {
    Inventory *out;
    int sizes, cancelled, category;
    ScanTick tick;
    void *context;
    const char *platform;
    Measure *measure;
    Game *games;
    size_t ngames, capacity;
} Scan;

static void read_error(Scan *s,const char *action,const char *path) {
    if (s->out->errors<8) diag_log("%s: %s: %s",action,path,strerror(errno));
    s->out->errors++;
}

static int ext_is(const char *name, const char *list) {
    const char *ext = strrchr(name, '.');
    if (!ext || !ext[1]) return 0;
    char key[32];
    size_t n = strlen(ext + 1);
    if (n + 3 > sizeof(key)) return 0;
    key[0] = ' ';
    for (size_t i = 0; i < n; i++) key[i + 1] = tolower((unsigned char)ext[i + 1]);
    key[n+1] = ' '; key[n+2] = 0;
    return strstr(list, key) != NULL;
}
static int equal(const char *a, const char *b) { return strcasecmp(a,b) == 0; }
static int metadata(const char *s) {
    return equal(s,"desktop.ini") || equal(s,"thumbs.db") || equal(s,"gamelist.xml")
        || equal(s,"readme") || ext_is(s," txt md nfo pdf log bak db xml ");
}
static int artwork(const char *s) {
    return equal(s,"Imgs") || equal(s,"images") || equal(s,"media") || equal(s,"thumbnails")
        || equal(s,"manuals") || equal(s,"videos") || equal(s,"boxart");
}
static int junk(const char *s) {
    return equal(s,"System Volume Information") || equal(s,"$RECYCLE.BIN") || equal(s,"lost+found");
}
static int playable(const char *name, const char *platform, int depth) {
    if ((equal(platform,"NEOGEO") || equal(platform,"ARCADE") || equal(platform,"MAME") ||
         equal(platform,"FBA2012") || equal(platform,"FBNEO")) &&
        (equal(name,"neogeo.zip") || equal(name,"pgm.zip") || equal(name,"qsound.zip") ||
         equal(name,"qsound_hle.zip"))) return 0;
    if (equal(platform,"PORTS") || equal(platform,"PORT")) return depth == 1 && ext_is(name," sh ");
    if (equal(platform,"SCUMMVM")) return ext_is(name," scummvm svm ") || (depth==1 && ext_is(name," sh "));
    if (equal(platform,"PICO") || equal(platform,"PICO8") || equal(platform,"PICO-8")) {
        size_t n=strlen(name);
        if (n>=7 && equal(name+n-7,".p8.png")) return 1;
    }
    return ext_is(name,
        " zip 7z nes fds unf unif smc sfc fig swc bs st gb gbc sgb gba agb gbx vb gw mgw "
        " md gen smd sms gg sg 32x pce sgx ngp ngc ngpc ws wsc lyn lnx a26 a52 a78 "
        " xfd atr atx xex car cas rom col int bin cue chd pbp m3u iso img ccd gdi cso "
        " dsk d64 d71 d81 t64 tap prg crt g64 adf adz lha hdf ipf chf cdi mx1 mx2 "
        " d88 2hd 88d hdm dim dx1 d77 xdf hdf hdi nfd nhd t88 cpr tzx sna szx z80 "
        " vec vpk wad pak p8 tic uzr uze min nds nds4 ndx jag j64 o2 mv cart fcpx "
        " aex nd romx psc dosz mgl thom k7 sap kcc msa stx opk mds n64 v64 z64 ");
}
static int checkpoint(Scan *s) {
    if (!s->cancelled && s->tick && !s->tick(s->context)) s->cancelled=1;
    return !s->cancelled;
}
static int append_path(char *dest, size_t cap, const char *dir, const char *name) {
    int n=snprintf(dest,cap,"%s/%s",dir,name);
    return n>=0 && (size_t)n<cap;
}
/* FAT may return DT_UNKNOWN. Full scans always request 64-bit sizes. */
static int entry_mode(Scan *s,const char *path,const struct dirent *e,int need_size,
                      uint64_t *bytes) {
    *bytes=0;
#ifdef _DIRENT_HAVE_D_TYPE
    if (!need_size && e->d_type!=DT_UNKNOWN) {
        if (e->d_type==DT_DIR) return 2;
        return e->d_type==DT_REG;
    }
#else
    (void)e; (void)need_size;
#endif
    struct stat st;
    if (lstat(path,&st)!=0) { read_error(s,"lstat",path); return 0; }
    if (S_ISDIR(st.st_mode)) return 2;
    if (!S_ISREG(st.st_mode)) return 0;
    *bytes=st.st_size<0?0:(uint64_t)st.st_size;
    return 1;
}
static void add_game(Scan *s,const char *path) {
    if (s->ngames==s->capacity) {
        size_t cap=s->capacity?2*s->capacity:64;
        Game *new_games=realloc(s->games,cap*sizeof(Game));
        if (!new_games) {diag_log("Not enough memory for the game list");s->cancelled=1;return;}
        s->games=new_games; s->capacity=cap;
    }
    char *copy=strdup(path);
    if (!copy) {diag_log("Not enough memory for a game path");s->cancelled=1;return;}
    s->games[s->ngames++]=(Game){copy,0};
}
static int compare_games(const void *a,const void *b) {
    return strcasecmp(((const Game *)a)->path,((const Game *)b)->path);
}
/* Resolve relative playlist references lexically; never open a descriptor-supplied path. */
static void mark_ref(Scan *s,const char *descriptor,char *name) {
    if (!*name || name[0]=='/' || name[0]=='\\' || strchr(name,':')) return;
    char path[PATH_MAX];
    size_t prefix=(size_t)(strrchr(descriptor,'/')+1-descriptor);
    if (prefix+strlen(name)>=sizeof(path)) return;
    memcpy(path,descriptor,prefix);
    size_t used=prefix;
    for (char *part=strtok(name,"/\\"); part; part=strtok(NULL,"/\\")) {
        if (!strcmp(part,".")) continue;
        if (!strcmp(part,"..")) {
            if (!used) return;
            --used;
            while(used && path[used-1]!='/') --used;
        } else {
            size_t n=strlen(part);
            memcpy(path+used,part,n); used+=n; path[used++]='/';
        }
    }
    if (!used) return;
    path[used-1]=0;
    Game key={path,0};
    Game *found=bsearch(&key,s->games,s->ngames,sizeof(Game),compare_games);
    if (found && !equal(path,descriptor)) found->referenced=1;
}
static void read_refs(Scan *s,const char *path) {
    int cue=ext_is(path," cue "), playlist=ext_is(path," m3u "), gdi=ext_is(path," gdi ");
    if (!cue && !playlist && !gdi) {
        if (ext_is(path," ccd mds ")) {
            char name[300];
            const char *base=strrchr(path,'/')+1;
            snprintf(name,sizeof(name),"%.*s.%s",(int)(strrchr(base,'.')-base),base,
                     ext_is(path," ccd ")?"img":"mdf");
            mark_ref(s,path,name);
        }
        return;
    }
    FILE *f=fopen(path,"rb");
    if (!f) { read_error(s,"open descriptor",path); return; }
    char line[PATH_MAX];
    while (fgets(line,sizeof(line),f) && checkpoint(s)) {
        char *p=line;
        if (!strncmp(p,"\xef\xbb\xbf",3)) p+=3;
        while (isspace((unsigned char)*p)) p++;
        char *end=p+strlen(p);
        while (end>p && isspace((unsigned char)end[-1])) *--end=0;
        if (!*p || *p=='#') continue;
        if (cue) {
            if (strncasecmp(p,"FILE",4) || !isspace((unsigned char)p[4])) continue;
            p+=4;
        } else if (gdi) {
            for (int i=0;i<4;i++) {
                while (isspace((unsigned char)*p)) p++;
                while (*p && !isspace((unsigned char)*p)) p++;
            }
        }
        while (isspace((unsigned char)*p)) p++;
        if (*p=='"') { p++; char *q=strchr(p,'"'); if (!q) continue; *q=0; }
        else if (cue) {
            /* The last field is the file type (BINARY, WAVE, ...). */
            char *q=p+strlen(p);
            while (q>p && !isspace((unsigned char)q[-1])) --q;
            while (q>p && isspace((unsigned char)q[-1])) --q;
            if (q==p) continue;
            *q=0;
        } else if (gdi) p[strcspn(p," \t")]=0;
        mark_ref(s,path,p);
    }
    if (ferror(f)) read_error(s,"read descriptor",path);
    fclose(f);
}
static void finish_games(Scan *s) {
    if (s->ngames) qsort(s->games,s->ngames,sizeof(Game),compare_games);
    for (size_t i=0;i<s->ngames && checkpoint(s);i++) read_refs(s,s->games[i].path);
    for (size_t i=0;i<s->ngames;i++) {
        if (!s->games[i].referenced) s->measure->count++;
        free(s->games[i].path);
    }
    free(s->games); s->games=NULL; s->ngames=s->capacity=0;
}
static int ignored_data(int category,const char *name) {
    if (category==SHOTS) return !ext_is(name," png jpg jpeg bmp webp ");
    return metadata(name) || ext_is(name," png jpg jpeg bmp webp ");
}
static void visit(Scan *s,const char *path,int depth,int assets) {
    if (depth>MAX_DEPTH) {s->out->errors++;return;}
    DIR *dir=opendir(path);
    if (!dir) {if (depth || errno!=ENOENT) read_error(s,"opendir",path);return;}
    struct dirent *e;
    for (;;) {
        errno=0;
        e=readdir(dir);
        if (!e) {if (errno) read_error(s,"readdir",path);break;}
        const char *name=e->d_name;
        if (!strcmp(name,".") || !strcmp(name,"..") || junk(name)) continue;
        int collection=s->category==GAMES || s->category==APPS;
        if (name[0]=='.' && !(collection && equal(name,".hidden"))) continue;
        if (!checkpoint(s)) break;
        char child[PATH_MAX];
        if (!append_path(child,sizeof(child),path,name)) {s->out->errors++;continue;}
        uint64_t size;
        int type=entry_mode(s,child,e,s->sizes,&size);
        if (type==2) {
            int data_only=collection && (assets || artwork(name) || name[0]=='.' || s->category==APPS);
            if (!s->sizes && data_only) continue;
            visit(s,child,depth+1,data_only);
        } else if (type==1) {
            if (s->category==GAMES || s->category==APPS) {
                if (s->sizes) s->measure->bytes+=size;
                if (assets || name[0]=='.') continue;
                if (s->category==GAMES && playable(name,s->platform,depth+1)) add_game(s,child);
                if (s->category==APPS && depth==0 && ext_is(name," sh ")) s->measure->count++;
            } else if (!ignored_data(s->category,name)) {
                s->measure->count++;
                if (s->sizes) s->measure->bytes+=size;
            }
        }
    }
    closedir(dir);
}
static int compare_platforms(const void *a,const void *b) {
    return strcasecmp(((const Platform *)a)->name,((const Platform *)b)->name);
}
static DIR *optional_dir(Scan *s,const char *path) {
    struct stat st;
    if (lstat(path,&st)!=0) { if (errno!=ENOENT) read_error(s,"lstat",path); return NULL; }
    if (!S_ISDIR(st.st_mode)) return NULL;
    DIR *d=opendir(path); if (!d) read_error(s,"opendir",path);
    return d;
}
static void scan_roms(Scan *s,const char *path) {
    DIR *dir=optional_dir(s,path); if (!dir) return;
    struct dirent *e;
    while (checkpoint(s)) {
        errno=0; e=readdir(dir);
        if (!e) {if (errno) read_error(s,"readdir",path);break;}
        const char *name=e->d_name;
        if (name[0]=='.' || artwork(name) || junk(name) || equal(name,"BIOS") ||
            equal(name,"MUSIC") || equal(name,"MEDIA") || equal(name,"PDF") || equal(name,"VIDEOS")) continue;
        char child[PATH_MAX]; uint64_t unused;
        if (!append_path(child,sizeof(child),path,name)) {s->out->errors++;continue;}
        if (entry_mode(s,child,e,0,&unused)!=2) continue;
        if (equal(name,"APPS")) {
            s->category=APPS; s->measure=&s->out->category[APPS]; visit(s,child,0,0); continue;
        }
        Platform p={0};
        snprintf(p.name,sizeof(p.name),"%s",name);
        s->category=GAMES; s->platform=p.name; s->measure=&p.files;
        visit(s,child,0,0); finish_games(s);
        s->out->category[GAMES].bytes+=p.files.bytes;
        s->out->category[GAMES].count+=p.files.count;
        if (p.files.count || p.files.bytes) {
            if (s->out->nplatforms<MAX_PLATFORMS) s->out->platforms[s->out->nplatforms++]=p;
            else s->out->errors++;
        }
    }
    closedir(dir);
    qsort(s->out->platforms,s->out->nplatforms,sizeof(Platform),compare_platforms);
}
static void group(Scan *s,const char *parent,const char *name,int cat) {
    char path[PATH_MAX];
    if (!append_path(path,sizeof(path),parent,name)) {s->out->errors++;return;}
    struct stat st;
    if (lstat(path,&st)<0) {if (errno!=ENOENT) read_error(s,"lstat",path);return;}
    if (!S_ISDIR(st.st_mode)) return;
    s->category=cat; s->measure=&s->out->category[cat];
    visit(s,path,0,0);
}
static void scan_saves(Scan *s,const char *root) {
    char path[PATH_MAX];
    if (!append_path(path,sizeof(path),root,"Saves")) {s->out->errors++;return;}
    DIR *d=optional_dir(s,path); if (!d) return;
    struct dirent *e;
    while (checkpoint(s)) {
        errno=0; e=readdir(d); if (!e) {if (errno) read_error(s,"readdir",path);break;}
        if (e->d_name[0]=='.' || junk(e->d_name)) continue;
        if (equal(e->d_name,"saves") || equal(e->d_name,"states")) {
            group(s,path,e->d_name,equal(e->d_name,"saves")?SAVES:STATES); continue;
        }
        char profile[PATH_MAX]; uint64_t unused;
        if (!append_path(profile,sizeof(profile),path,e->d_name)) {s->out->errors++;continue;}
        if (entry_mode(s,profile,e,0,&unused)==2) {
            group(s,profile,"saves",SAVES); group(s,profile,"states",STATES);
        }
    }
    closedir(d);
}
int scan_card(const char *root,Inventory *out,int sizes,ScanTick tick,void *context) {
    memset(out,0,sizeof(*out));
    Scan s={.out=out,.sizes=sizes,.tick=tick,.context=context};
    char path[PATH_MAX];
    if (!append_path(path,sizeof(path),root,"Roms")) return 0;
    scan_roms(&s,path);
    if (checkpoint(&s)) scan_saves(&s,root);
    if (checkpoint(&s)) group(&s,root,"Screenshots",SHOTS);
    if (checkpoint(&s)) group(&s,root,"CFW/retroarch/.retroarch/screenshots",SHOTS);
    if (checkpoint(&s)) group(&s,root,"BIOS",BIOS);
    return !s.cancelled;
}

