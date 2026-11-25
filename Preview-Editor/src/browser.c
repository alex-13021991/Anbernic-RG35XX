#include "browser.h"
#include "editor.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
static int compare(const void *a,const void *b) {
    const Entry *x=a,*y=b;
    if (x->folder!=y->folder) return y->folder-x->folder;
    int s=strcasecmp(x->name,y->name);
    return s ? s : strcmp(x->name,y->name);
}
void browser_clear(Browser *b) {
    for (int i=0;i<b->count;i++) free(b->entries[i].name);
    free(b->entries); b->entries=NULL; b->count=0; b->row=0;
}
int browser_enter(Browser *b,const char *folder,const char *select_name) {
    DIR *dir=opendir(folder);
    if (!dir) return fail("Каталог недоступен. Проверьте SD-карту.");
    Browser tmp={0};
    int capacity=0,ok=1;
    for (;;) {
        errno=0;
        struct dirent *e=readdir(dir);
        if (!e) { if (errno) ok=0; break; }
        if (e->d_name[0]=='.') continue;
        struct stat st;
        if (fstatat(dirfd(dir),e->d_name,&st,AT_SYMLINK_NOFOLLOW)) continue;
        int directory=S_ISDIR(st.st_mode);
        const char *ext=strrchr(e->d_name,'.');
        if (!directory && !(S_ISREG(st.st_mode) && ext && !strcasecmp(ext,".png"))) continue;
        if (tmp.count==capacity) {
            capacity=capacity ? capacity*2 : 32;
            Entry *p=realloc(tmp.entries,(size_t)capacity*sizeof *p);
            if (!p) { ok=0; break; }
            tmp.entries=p;
        }
        char *name=strdup(e->d_name);
        if (!name) { ok=0; break; }
        tmp.entries[tmp.count++]=(Entry){name,directory};
    }
    closedir(dir);
    if (!ok) { browser_clear(&tmp); return fail("Не удалось прочитать список файлов."); }
    if (tmp.count>1) qsort(tmp.entries,tmp.count,sizeof *tmp.entries,compare);
    if (select_name)
        for (int i=0;i<tmp.count;i++) if (!strcasecmp(tmp.entries[i].name,select_name)) tmp.row=i;
    /* select_name and folder can point into the old listing. */
    snprintf(tmp.folder,sizeof tmp.folder,"%s",folder);
    browser_clear(b);
    strcpy(b->folder,tmp.folder); b->entries=tmp.entries; b->count=tmp.count; b->row=tmp.row;
    return 1;
}
int browser_back(Browser *b) {
    if (!strcmp(b->root[b->card],b->folder)) {
        browser_clear(b); b->folder[0]=0; b->row=b->card;
        return 1;
    }
    char parent[PATH_MAX]; strcpy(parent,b->folder);
    char *slash=strrchr(parent,'/');
    if (!slash) return fail("Не удалось открыть родительский каталог.");
    *slash=0;
    return browser_enter(b,parent,slash+1);
}
int browser_path(const Browser *b,char out[PATH_MAX]) {
    return b->count>0 && (snprintf(out,PATH_MAX,"%s/%s",b->folder,b->entries[b->row].name)<PATH_MAX ||
        fail("Слишком длинный путь."));
}
