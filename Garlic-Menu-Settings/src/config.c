#include "gms.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Settings defaults(void) { return (Settings){LEFT,16,313,1,40,1000,1000}; }

/* Raw numbers preserve theme extensions even when their precision exceeds double. */
yyjson_doc *json_object(const Blob *b) {
    yyjson_doc *doc=yyjson_read(b->data,b->len,YYJSON_READ_NUMBER_AS_RAW);
    yyjson_val *root=yyjson_doc_get_root(doc);
    if (yyjson_is_obj(root)) {
        size_t i,n;
        yyjson_val *k,*v;
        yyjson_obj_foreach(root,i,n,k,v) {
            const char *name=yyjson_get_str(k);
            if (strlen(name)!=yyjson_get_len(k) || yyjson_obj_get(root,name)!=v) break;
        }
        if (i==n) return doc;
    }
    yyjson_doc_free(doc);
    fail("Ожидается корректный объект JSON с уникальными ключами.");
    return NULL;
}

static int number(yyjson_val *v,int min,int max,int fallback) {
    const char *raw=yyjson_get_raw(v);
    if (!raw) return fallback;
    char *end;
    errno=0;
    long val=strtol(raw,&end,10);
    return errno || *end || val<min || val>max ? fallback : (int)val;
}
int settings_read(const Blob *b,Settings *s) {
    yyjson_doc *d=json_object(b);
    if (!d) return 0;
    yyjson_val *obj=yyjson_doc_get_root(d);
    *s=defaults();
    const char *align=yyjson_get_str(yyjson_obj_get(obj,"text-alignment"));
    s->align=align && !strcmp(align,"left") ? LEFT :
             align && !strcmp(align,"right") ? RIGHT : CENTER;
    s->margin=number(yyjson_obj_get(obj,"text-margin"),0,32767,16);
    s->width=number(yyjson_obj_get(obj,"text-max-width"),0,640,313);
    yyjson_val *scroll=yyjson_obj_get(obj,"text-scroll");
    if (yyjson_is_bool(scroll)) s->scroll=yyjson_get_bool(scroll);
    s->speed=number(yyjson_obj_get(obj,"text-scroll-speed"),10,200,40);
    s->delay=number(yyjson_obj_get(obj,"text-scroll-delay"),0,10000,1000);
    s->pause=number(yyjson_obj_get(obj,"text-scroll-pause"),0,10000,1000);
    yyjson_doc_free(d);
    return 1;
}

typedef struct { char *p; size_t used,cap; } Buffer;
static int append(Buffer *buf,const char *s,size_t n) {
    if (buf->used+n>=FILE_LIMIT) return fail("JSON превышает допустимый размер.");
    if (buf->used+n+1>buf->cap) {
        size_t cap=(buf->used+n+1)*2;
        char *p=realloc(buf->p,cap);
        if (!p) return fail("Недостаточно памяти для JSON.");
        buf->p=p; buf->cap=cap;
    }
    memcpy(buf->p+buf->used,s,n);
    buf->used+=n; buf->p[buf->used]=0;
    return 1;
}

/* v0.5 reads one top-level key per line, and stock margin needs a space after ':'.
   Emit unknown values compactly on their own member's line, including nested objects. */
static int member(Buffer *buf,yyjson_val *k,yyjson_val *v) {
    size_t kn=0,vn=0;
    char *key=yyjson_val_write(k,0,&kn);
    char *value=yyjson_val_write(v,0,&vn);
    int ok=key && value &&
        append(buf,buf->used>1 ? ",\n\t" : "\n\t",buf->used>1 ? 3 : 2) &&
        append(buf,key,kn) && append(buf,": ",2) &&
        append(buf,value,vn);
    free(key); free(value);
    return ok;
}

int settings_write(const Blob *old,const Blob *add,const Settings *edit,Blob *out) {
    yyjson_doc *doc=json_object(old);
    if (!doc) return 0;
    static const char *names[]={"text-alignment","text-margin","text-max-width","text-scroll"};
    char edits[192];
    if (edit) {
        const char *align=edit->align==LEFT ? "left" : edit->align==RIGHT ? "right" : "center";
        snprintf(edits,sizeof edits,"{\"%s\":\"%s\",\"%s\":%d,\"%s\":%d,\"%s\":%s}",
                 names[0],align,names[1],edit->margin,names[2],edit->width,names[3],edit->scroll?"true":"false");
    }
    Blob changes={0};
    if (edit) { changes.data=edits; changes.len=strlen(edits); }
    yyjson_doc *delta=json_object(edit ? &changes : add);
    yyjson_val *base=yyjson_doc_get_root(doc),*updates=yyjson_doc_get_root(delta);
    Buffer buf={0};
    int ok=delta && append(&buf,"{",1);
    size_t i,n;
    yyjson_val *k,*v;
    yyjson_obj_foreach(base,i,n,k,v) {
        yyjson_val *replacement=edit ? yyjson_obj_get(updates,yyjson_get_str(k)) : NULL;
        if (ok) ok=member(&buf,k,replacement ? replacement : v);
    }
    yyjson_obj_foreach(updates,i,n,k,v) {
        if (ok && !yyjson_obj_get(base,yyjson_get_str(k))) ok=member(&buf,k,v);
    }
    ok=ok && append(&buf,"\n}\n",3);
    yyjson_doc_free(delta); yyjson_doc_free(doc);
    if (!ok) { free(buf.p); return fail("Не удалось подготовить JSON."); }
    *out=(Blob){.data=buf.p,.len=buf.used,.mode=old->mode};
    fingerprint(out);
    return 1;
}

Row layout(Settings s,int cover,int width,int selected,unsigned elapsed) {
    Row r={.x=(640-width)/2,.width=width};
    if (s.align==RIGHT) r.x=640-s.margin-width;
    if (s.align!=LEFT) return r;
    r.x=s.margin;
    if (!cover || !s.width) return r;
    int limit=s.width<640-r.x ? s.width : 640-r.x;
    if (limit<=0) { r.width=0; return r; }
    if (width<=limit) return r;
    r.width=limit;
    if (selected && s.scroll) {
        unsigned distance=width-limit;
        unsigned transit=(distance*1000u+s.speed-1)/s.speed;
        unsigned t=elapsed%(s.delay+transit+s.pause);
        if (t>(unsigned)s.delay) {
            unsigned shift=(t-s.delay)*(unsigned)s.speed/1000u;
            r.offset=shift>distance ? (int)distance : (int)shift;
        }
    }
    return r;
}

