#include "editor.h"
#include "sha256.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

char problem[256];
int fail(const char *s) {
    snprintf(problem,sizeof problem,"%s",s);
    return 0;
}
static int combine(char out[PATH_MAX],const char *dir,const char *name) {
    return snprintf(out,PATH_MAX,"%s/%s",dir,name)<PATH_MAX || fail("Слишком длинный путь.");
}
static FILE *read_regular(const char *path) {
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if (fd<0) return NULL;
    struct stat st;
    FILE *f=NULL;
    if (!fstat(fd,&st) && S_ISREG(st.st_mode)) f=fdopen(fd,"rb");
    if (!f) close(fd);
    return f;
}
static int stream(FILE *src,FILE *dst,Digest sum) {
    SHA256_CTX hash;
    uint8_t bytes[16384];
    if (fseek(src,0,SEEK_SET)) return fail("Не удалось прочитать файл на карте.");
    sha256_init(&hash);
    size_t n;
    while ((n=fread(bytes,1,sizeof bytes,src))>0) {
        if (dst && fwrite(bytes,1,n,dst)!=n) return fail("Недостаточно места или ошибка записи.");
        sha256_update(&hash,bytes,n);
    }
    if (ferror(src)) return fail("Ошибка чтения карты.");
    sha256_final(&hash,sum);
    return 1;
}
static int fingerprint(const char *path,Digest sum) {
    FILE *f=read_regular(path);
    if (!f) return fail("Файл недоступен для чтения.");
    int ok=stream(f,NULL,sum);
    fclose(f);
    return ok;
}
int load_document(const char *path,Image *out,Digest version) {
    FILE *f=read_regular(path);
    if (!f) return fail("Не удалось открыть выбранный PNG.");
    int ok=stream(f,NULL,version) && !fseek(f,0,SEEK_SET) && read_png(f,out);
    fclose(f);
    return ok;
}
typedef struct { char dir[PATH_MAX],backdir[PATH_MAX],backup[PATH_MAX]; } Paths;
static int paths_for(const char *path,Paths *p) {
    const char *slash=strrchr(path,'/');
    size_t len=slash ? (size_t)(slash-path) : 0;
    if (len>=sizeof p->dir) return fail("Слишком длинный путь.");
    if (slash && len) { memcpy(p->dir,path,len); p->dir[len]=0; }
    else strcpy(p->dir,slash ? "/" : ".");
    return combine(p->backdir,p->dir,".preview-originals") &&
        combine(p->backup,p->backdir,slash ? slash+1 : path);
}
int has_original(const char *path) {
    Paths p; struct stat st;
    return paths_for(path,&p) && !lstat(p.backdir,&st) && S_ISDIR(st.st_mode) &&
        !lstat(p.backup,&st) && S_ISREG(st.st_mode);
}
static int sync_dir(const char *dir) {
    int fd=open(dir,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    int ok=fd>=0 && fsync(fd)==0;
    if (fd>=0) close(fd);
    return ok || fail("Карта не подтвердила запись каталога.");
}
typedef struct { char name[PATH_MAX]; FILE *f; } Temp;
static int start_temp(Temp *t,const char *dir) {
    char name[PATH_MAX];
    if (!combine(name,dir,".pe-XXXXXX")) return 0;
    int fd=mkstemp(name);
    if (fd<0) return fail("Не удалось создать временный файл. Проверьте карту.");
    strcpy(t->name,name);
    t->f=fdopen(fd,"w+b");
    if (!t->f) { close(fd); return fail("Не удалось открыть временный файл."); }
    return 1;
}
static void discard(Temp *t) {
    if (t->f) fclose(t->f);
    if (*t->name) unlink(t->name);
}
static int flush_temp(Temp *t) {
    return (!fflush(t->f) && !fsync(fileno(t->f))) || fail("Карта не подтвердила запись файла.");
}
static int publish(Temp *t,const char *path,const char *dir) {
    int ok=fclose(t->f)==0;
    t->f=NULL;
    if (!ok || rename(t->name,path)) return fail("Ошибка замены файла на карте.");
    t->name[0]=0;
    return sync_dir(dir);
}
static int copy_verified(const char *from,const char *to,const char *dir,const uint8_t *expected) {
    FILE *src=read_regular(from);
    if (!src) return fail("Исходный файл недоступен.");
    Digest a,b;
    Temp t={0};
    Image check={0};
    int ok=start_temp(&t,dir) && stream(src,t.f,a) && flush_temp(&t) && stream(t.f,NULL,b);
    fclose(src);
    if (ok && (memcmp(a,b,sizeof a) || (expected && memcmp(a,expected,sizeof a))))
        ok=fail("Копия отличается от оригинала.");
    if (ok) ok=!fseek(t.f,0,SEEK_SET) && read_png(t.f,&check);
    free_image(&check);
    if (ok) ok=publish(&t,to,dir);
    discard(&t);
    return ok;
}
static int backup_original(const char *path,const Paths *p,const Digest expected) {
    struct stat st;
    if (mkdir(p->backdir,0700) && errno!=EEXIST) return fail("Не удалось создать папку оригиналов.");
    if (lstat(p->backdir,&st) || !S_ISDIR(st.st_mode)) return fail("Папка оригиналов недоступна.");
    /* Sync even when retrying an interrupted first save. */
    if (!sync_dir(p->dir)) return 0;
    if (lstat(p->backup,&st)) {
        if (errno!=ENOENT) return fail("Не удалось проверить наличие оригинала.");
        return copy_verified(path,p->backup,p->backdir,expected);
    }
    FILE *f=read_regular(p->backup);
    Image test={0};
    int ok=f && read_png(f,&test);
    if (f) fclose(f);
    free_image(&test);
    return (ok && sync_dir(p->backdir)) || fail("Не удалось подтвердить резервную копию.");
}
static int lock_folder(const Paths *p) {
    int fd=open(p->dir,O_RDONLY|O_DIRECTORY);
    if (fd>=0 && flock(fd,LOCK_EX|LOCK_NB)==0) return fd;
    if (fd>=0) close(fd);
    fail("Папка занята или карта недоступна.");
    return -1;
}
static int unchanged(const char *path,const Digest expected) {
    Digest now;
    return fingerprint(path,now) && (!memcmp(now,expected,sizeof now) ||
        fail("PNG изменился на карте. Откройте его заново."));
}
int save_document(const char *path,const Pixel *canvas,Digest version) {
    Paths p;
    if (!paths_for(path,&p)) return 0;
    int lock=lock_folder(&p);
    if (lock<0) return 0;
    Temp t={0};
    Image out={CW,CH,(Pixel *)canvas},check={0};
    Digest next;
    int ok=unchanged(path,version) && start_temp(&t,p.dir) && write_png(t.f,&out) &&
        flush_temp(&t) && !fseek(t.f,0,SEEK_SET) && read_png(t.f,&check);
    if (ok && (check.w!=CW || check.h!=CH || memcmp(check.p,canvas,CW*CH*sizeof(Pixel))))
        ok=fail("Проверка записанного PNG не пройдена.");
    free_image(&check);
    if (ok) ok=stream(t.f,NULL,next) && backup_original(path,&p,version) && unchanged(path,version);
    if (ok) {
        ok=publish(&t,path,p.dir);
        if (!*t.name) memcpy(version,next,sizeof next);
    }
    discard(&t);
    close(lock);
    return ok;
}
int restore_document(const char *path) {
    Paths p;
    if (!paths_for(path,&p)) return 0;
    int lock=lock_folder(&p);
    if (lock<0) return 0;
    int ok=has_original(path) ? copy_verified(p.backup,path,p.dir,NULL) : fail("Резервная копия отсутствует.");
    close(lock);
    return ok;
}
Prefs load_prefs(void) {
    Prefs p={16,-1,-1},v;
    FILE *f=read_regular("preview.cfg");
    if (f) {
        if (fscanf(f,"margin=%d top=%d bottom=%d",&v.margin,&v.top,&v.bottom)==3 &&
            v.margin>=0 && v.margin<=120 && v.top>=-1 && v.top<=120 && v.bottom>=-1 && v.bottom<=120) p=v;
        fclose(f);
    }
    return p;
}
int save_prefs(Prefs p) {
    Temp t={0};
    int ok=start_temp(&t,".") && fprintf(t.f,"margin=%d\ntop=%d\nbottom=%d\n",p.margin,p.top,p.bottom)>0 &&
        flush_temp(&t) && publish(&t,"preview.cfg",".");
    discard(&t);
    return ok;
}
