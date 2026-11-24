#include "gms.h"
#include "sha256.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

char problem[512];
int fail(const char *fmt,...) {
    va_list ap; va_start(ap,fmt); vsnprintf(problem,sizeof problem,fmt,ap); va_end(ap);
    return 0;
}
void fingerprint(Blob *b) {
    SHA256_CTX ctx; unsigned char digest[32];
    sha256_init(&ctx); sha256_update(&ctx,(const unsigned char *)b->data,b->len);
    sha256_final(&ctx,digest);
    for (size_t i=0;i<sizeof digest;i++) sprintf(b->hash+2*i,"%02x",digest[i]);
}
int exists(const char *path) {
    struct stat st;
    if (lstat(path,&st)==0) return 1;
    if (errno==ENOENT) return 0;
    fail("Не удалось проверить %s (%d).",path,errno);
    return -1;
}
int read_blob(const char *path,Blob *b) {
    struct stat st;
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if (fd<0) return fail("Не удалось открыть %s (%d).",path,errno);
    int ok=fstat(fd,&st)==0 && S_ISREG(st.st_mode) && st.st_size>=0 && st.st_size<=(off_t)FILE_LIMIT;
    if (ok) {
        b->len=(size_t)st.st_size; b->mode=st.st_mode&0777;
        b->data=malloc(b->len+1);
        ok=b->data!=NULL;
    }
    size_t pos=0;
    while (ok && pos<b->len) {
        ssize_t n=read(fd,b->data+pos,b->len-pos);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) ok=0; else pos+=(size_t)n;
    }
    char extra;
    if (ok) ok=read(fd,&extra,1)==0;
    if (close(fd)) ok=0;
    if (!ok) { free(b->data); b->data=NULL; return fail("Ошибка чтения или типа файла %s.",path); }
    b->data[b->len]=0; fingerprint(b);
    return 1;
}
int load_package(Package *p) {
    Blob manifest={0};
    if (!read_blob("patch/manifest.json",&manifest)) return 0;
    yyjson_doc *doc=json_object(&manifest);
    free(manifest.data);
    if (!doc) return 0;
    const char *keys[]={"stock_main_sha256","patched_main_sha256","additions_sha256"};
    const char *expected[]={STOCK,PATCHED,ADDITIONS};
    int ok=1;
    for (int i=0;i<3;i++) {
        const char *v=yyjson_get_str(yyjson_obj_get(yyjson_doc_get_root(doc),keys[i]));
        if (!v || strcmp(v,expected[i])) ok=0;
    }
    yyjson_doc_free(doc);
    if (!ok) return fail("Хеши манифеста отличаются от версии 1.0.");
    if (!read_blob("patch/main",&p->main) || !read_blob("patch/settings-additions.json",&p->additions)) return 0;
    return (!strcmp(p->main.hash,PATCHED) && !strcmp(p->additions.hash,ADDITIONS)) ||
           fail("Контрольная сумма файлов патча v0.5 не совпала.");
}
int enter_system(const char *cfw) {
    static int lockfd=-1;
    int fd=open(cfw,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if (fd<0) return fail("Системная папка CFW на TF1 недоступна.");
    if (flock(fd,LOCK_EX|LOCK_NB) || fchdir(fd)) {
        close(fd); return fail("Системная папка занята или недоступна.");
    }
    struct stat skin,base;
    if (lstat("skin",&skin) || fstat(fd,&base) || !S_ISDIR(skin.st_mode) || skin.st_dev!=base.st_dev) {
        close(fd); return fail("Папка CFW/skin недоступна на системной карте.");
    }
    if (lockfd>=0) close(lockfd);
    lockfd=fd;
    return 1;
}

