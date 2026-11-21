#include "gallery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
typedef struct { dev_t dev; ino_t ino; } Seen;
typedef struct { Gallery *g; Seen dirs[2048]; size_t ndirs; char cards[2][PATH_MAX]; } Scan;
static int inside(const char *path,const char *root) {
    size_t n=strlen(root); return !strncmp(path,root,n) && (path[n]=='/'||path[n]==0);
}
const char *shot_name(const Shot *s) { const char *p=strrchr(s->path,'/'); return p?p+1:s->path; }
static int sort_shots(const void *aa,const void *bb) {
    const Shot *a=aa,*b=bb;
    if(a->stamp.st_mtime!=b->stamp.st_mtime) return a->stamp.st_mtime>b->stamp.st_mtime?-1:1;
    int n=strcmp(shot_name(a),shot_name(b)); if(n) return n;
    return strcmp(a->path,b->path);
}
static void walk(Scan *s,const char *path,int card,int depth) {
    struct stat st; if(lstat(path,&st)<0) {if(errno!=ENOENT) s->g->errors++;return;}
    if(S_ISLNK(st.st_mode)) return;
    if(S_ISDIR(st.st_mode)) {
        if(depth>12 || s->ndirs>=2048) {s->g->limited=1;return;}
        for(size_t i=0;i<s->ndirs;i++) if(s->dirs[i].dev==st.st_dev && s->dirs[i].ino==st.st_ino) return;
        s->dirs[s->ndirs++]=(Seen){st.st_dev,st.st_ino};
        DIR *d=opendir(path); if(!d) {s->g->errors++;return;}
        struct dirent *e;
        while((e=readdir(d))) {
            if(e->d_name[0]=='.') continue;
            char p[PATH_MAX]; int n=snprintf(p,sizeof(p),"%s/%s",path,e->d_name);
            if(n<0 || n>=(int)sizeof(p)) {s->g->errors++;continue;}
            walk(s,p,card,depth+1);
            if(s->g->count>=MAX_SHOTS) {s->g->limited=1;break;}
        }
        closedir(d); return;
    }
    if(!S_ISREG(st.st_mode)) return;
    const char *ext=strrchr(path,'.'); if(!ext||strcasecmp(ext,".png")) return;
    for(size_t i=0;i<s->g->count;i++) {
        Shot *p=&s->g->shots[i];
        if(!strcmp(p->path,path) || (p->stamp.st_dev==st.st_dev && p->stamp.st_ino==st.st_ino)) return;
    }
    if(s->g->count>=MAX_SHOTS) {s->g->limited=1;return;}
    char *p=strdup(path); if(!p) {s->g->limited=1;return;}
    s->g->shots[s->g->count++]=(Shot){p,card,st};
}
static void add_root(Scan *s,const char *path) {
    char real[PATH_MAX]; if(!realpath(path,real)) {if(errno!=ENOENT) s->g->errors++;return;}
    for(int i=0;i<2;i++) if(s->cards[i][0] && inside(real,s->cards[i])) {walk(s,real,i+1,0);return;}
    /* Only files on the two known cards may enter the deletion-capable gallery. */
    s->g->errors++;
}
static void read_ra(Scan *s,const char *cfg) {
    FILE *f=fopen(cfg,"r"); if(!f) return;
    char line[PATH_MAX+100];
    while(fgets(line,sizeof(line),f)) {
        char *p=line; while(isspace((unsigned char)*p)) ++p;
        const char *name="screenshot_directory"; size_t n=strlen(name);
        if(strncmp(p,name,n)) continue; p+=n;
        while(isspace((unsigned char)*p)) ++p; if(*p++!='=') continue;
        while(isspace((unsigned char)*p)) ++p;
        if(*p=='"') { ++p;char *end=strchr(p,'"');if(!end) continue;*end=0; }
        else {char *end=p+strcspn(p,"\r\n#");while(end>p && isspace((unsigned char)end[-1]))--end;*end=0;}
        if(*p=='/') add_root(s,p);
    }
    fclose(f);
}
int gallery_scan(Gallery *g,const char *c1,const char *c2,const char *extra) {
    memset(g,0,sizeof(*g)); g->shots=calloc(MAX_SHOTS,sizeof(Shot)); if(!g->shots) return 0;
    Scan s; memset(&s,0,sizeof(s)); s.g=g;
    const char *cards[]={c1,c2};
    for(int i=0;i<2;i++) if(!realpath(cards[i],s.cards[i])) s.cards[i][0]=0;
    const char *dirs[]={"Screenshots","screenshots","CFW/retroarch/.retroarch/screenshots"};
    for(int i=0;i<2;i++) if(s.cards[i][0]) {
        char p[PATH_MAX];
        for(size_t j=0;j<sizeof(dirs)/sizeof(*dirs);j++) {
            snprintf(p,sizeof(p),"%s/%s",s.cards[i],dirs[j]); add_root(&s,p);
        }
        snprintf(p,sizeof(p),"%s/CFW/retroarch/.retroarch/retroarch.cfg",s.cards[i]); read_ra(&s,p);
        snprintf(p,sizeof(p),"%s/CFW/retroarch/retroarch.cfg",s.cards[i]); read_ra(&s,p);
    }
    FILE *f=extra?fopen(extra,"r"):NULL;
    if(f) {char line[PATH_MAX];while(fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\r\n")]=0; if(line[0]=='/') add_root(&s,line);
    } fclose(f);}
    qsort(g->shots,g->count,sizeof(Shot),sort_shots); return 1;
}
void gallery_free(Gallery *g) {
    for(size_t i=0;i<g->count;i++) free(g->shots[i].path);
    free(g->shots);memset(g,0,sizeof(*g));
}
int gallery_delete(Gallery *g,size_t index) {
    if(index>=g->count) {errno=EINVAL;return -1;}
    Shot *s=&g->shots[index]; char parent[PATH_MAX],real[PATH_MAX];
    if(strlen(s->path)>=sizeof(parent)) {errno=ENAMETOOLONG;return -1;}
    strcpy(parent,s->path);char *slash=strrchr(parent,'/'); if(!slash) {errno=EINVAL;return -1;} *slash=0;
    /* A replaced/symlinked parent must not redirect a delete to another tree. */
    if(!realpath(parent,real)||strcmp(real,parent)) {errno=ESTALE;return -1;}
    int fd=open(parent,O_RDONLY|O_DIRECTORY|O_NOFOLLOW); if(fd<0) return -1;
    struct stat now; const char *name=shot_name(s); int rc=-1;
    if(fstatat(fd,name,&now,AT_SYMLINK_NOFOLLOW)<0) goto done;
    if(!S_ISREG(now.st_mode) || now.st_dev!=s->stamp.st_dev || now.st_ino!=s->stamp.st_ino ||
       now.st_size!=s->stamp.st_size || now.st_mtime!=s->stamp.st_mtime || now.st_ctime!=s->stamp.st_ctime) {errno=ESTALE;goto done;}
    rc=unlinkat(fd,name,0);
done:;
    int error=errno;close(fd);errno=error;
    if(rc==0) {free(s->path);memmove(s,s+1,(g->count-index-1)*sizeof(*s));--g->count;}
    return rc;
}
static uint32_t be32(const unsigned char *p) {return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int png_dimensions(const char *path,int *w,int *h) {
    unsigned char b[24]; struct stat st;
    if(lstat(path,&st)<0 || !S_ISREG(st.st_mode) || st.st_size>32*1024*1024 || st.st_size<24) return 0;
    FILE *f=fopen(path,"rb"); if(!f) return 0; size_t n=fread(b,1,sizeof(b),f);fclose(f);
    if(n!=24||memcmp(b,"\x89PNG\r\n\x1a\n",8)||memcmp(b+12,"IHDR",4)||be32(b+8)!=13) return 0;
    uint32_t x=be32(b+16),y=be32(b+20);
    if(!x||!y||x>4096||y>4096||(uint64_t)x*y>4194304) return 0;
    *w=(int)x;*h=(int)y; return 1;
}
