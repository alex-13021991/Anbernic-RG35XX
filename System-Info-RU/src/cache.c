#include "cache.h"
#include "diag.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char magic[8]="SIRU001";
static uint32_t checksum(const Report *r) {
    uint32_t hash=2166136261u;
    const unsigned char *p=(const unsigned char *)r;
    for (size_t i=0;i<sizeof(*r);i++) hash=(hash^p[i])*16777619u;
    return hash;
}
int report_matches(const Report *r,const Volume v[2]) {
    for (int i=0;i<2;i++) {
        if (r->present[i]!=(unsigned)v[i].present ||
            memcmp(r->cid[i],v[i].cid,sizeof(v[i].cid))) return 0;
    }
    return 1;
}
static int cacheable(const Report *r) {
    if (!r->detailed || !r->completed || (!r->present[0] && !r->present[1])) return 0;
    for (int i=0;i<2;i++) {
        const Inventory *c=&r->cards[i];
        if ((r->present[i] && !r->cid[i][0]) || c->errors || c->nplatforms>MAX_PLATFORMS) return 0;
        for (unsigned j=0;j<c->nplatforms;j++)
            if (!memchr(c->platforms[j].name,0,sizeof(c->platforms[j].name))) return 0;
    }
    return 1;
}
int cache_load(Report *r,const Volume v[2]) {
    FILE *f=fopen("inventory.cache","rb");
    if (!f) {diag_log("Cache open: %s",strerror(errno));return 0;}
    char tag[8]; uint32_t stamp;
    int valid=fread(tag,1,sizeof(tag),f)==sizeof(tag) && !memcmp(tag,magic,sizeof(tag)) &&
        fread(&stamp,sizeof(stamp),1,f)==1 && fread(r,sizeof(*r),1,f)==1 &&
        fgetc(f)==EOF && !ferror(f) && checksum(r)==stamp && cacheable(r) && report_matches(r,v);
    fclose(f);
    if (!valid) memset(r,0,sizeof(*r));
    diag_log("Cache %s",valid?"loaded":"invalid or belongs to another card set");
    return valid;
}
int cache_save(const Report *r) {
    if (!cacheable(r)) {diag_log("Cache skipped: incomplete scan or unavailable card ID");return 0;}
    char path[]="inventory.XXXXXX";
    int fd=mkstemp(path);
    if (fd<0) {diag_log("Cache create: %s",strerror(errno));return 0;}
    FILE *f=fdopen(fd,"wb");
    if (!f) {diag_log("Cache fdopen: %s",strerror(errno));close(fd);unlink(path);return 0;}
    uint32_t stamp=checksum(r);
    int ok=fwrite(magic,1,sizeof(magic),f)==sizeof(magic) &&
        fwrite(&stamp,sizeof(stamp),1,f)==1 && fwrite(r,sizeof(*r),1,f)==1 &&
        fflush(f)==0 && fsync(fd)==0;
    if (fclose(f)!=0) ok=0;
    if (ok && rename(path,"inventory.cache")==0) {diag_log("Cache saved");return 1;}
    diag_log("Cache write/rename: %s",strerror(errno));
    unlink(path); return 0;
}

