#include "gms.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define WORK ".garlic-menu.work"
#define PLAN WORK "/plan"
#define DONE WORK "/done"
#define TEMP WORK "/record.tmp"
#define RECEIPT ".garlic-menu.original"
static const char *live[2]={MAIN,CFG};
static const char *back[2]={BACK_MAIN,BACK_CFG};
static const char *before[2]={WORK "/before-main",WORK "/before-json"};
static const char *after[2]={WORK "/after-main",WORK "/after-json"};
typedef struct { char op; Hash old[2],next[2],original; } Journal;

/* The test build can lose power after a durable step or fail a single I/O operation. */
#ifdef GMS_TEST
static int step;
static int checkpoint(void) {
    const char *crash=getenv("GMS_CRASH"), *error=getenv("GMS_ERROR");
    ++step;
    if (crash && atoi(crash)==step) _exit(85);
    if (error && atoi(error)==step) return fail("Тестовая ошибка записи.");
    return 1;
}
#else
#define checkpoint() 1
#endif
static int digest_ok(const char *s) {
    return strlen(s)==64 && strspn(s,"0123456789abcdef")==64;
}
static int same(const char *file,const char *hash) {
    Blob b={0}; if (!read_blob(file,&b)) return 0;
    int ok=!strcmp(b.hash,hash); free(b.data);
    return ok || fail("Контрольная сумма %s не совпала.",file);
}
static int dir_sync(const char *path) {
    int fd=open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if (fd<0) return fail("Каталог %s недоступен.",path);
    int ok=fsync(fd)==0; close(fd);
    return ok || fail("Не удалось синхронизировать каталог %s (%d).",path,errno);
}
static int erase(const char *p) {
    if (!unlink(p)) return checkpoint();
    return errno==ENOENT || fail("Не удалось удалить %s (%d).",p,errno);
}
static int create(const char *p,const Blob *b,mode_t mode) {
    int fd=open(p,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,mode);
    if (fd<0) return fail("Не удалось создать %s (%d).",p,errno);
    int ok=1;
    size_t pos=0;
    while (pos<b->len) {
        ssize_t n=write(fd,b->data+pos,b->len-pos);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) { ok=0; break; }
        pos+=(size_t)n;
    }
    /* FAT exports mount-wide modes. Creation mode suffices there. */
    struct stat st;
    if (ok && (fstat(fd,&st) || ((st.st_mode&mode)!=mode && fchmod(fd,mode)))) ok=0;
    if (ok && fsync(fd)) ok=0;
    if (close(fd)) ok=0;
    return (ok || fail("Ошибка записи %s (%d).",p,errno)) && checkpoint();
}
static int move(const char *from,const char *to) {
    return (!rename(from,to) || fail("Не удалось заменить %s (%d).",to,errno)) &&
           dir_sync(".") && dir_sync("skin") && dir_sync(WORK) && checkpoint();
}
static int receipt(Hash out) {
    Blob b={0}; if (!read_blob(RECEIPT,&b)) return 0;
    int ok=b.len==64 && digest_ok(b.data);
    if (ok) memcpy(out,b.data,65);
    free(b.data);
    return ok || fail("Повреждена запись о резервных копиях.");
}
static int backups(const char *jsonhash) {
    return same(BACK_MAIN,STOCK) && same(BACK_CFG,jsonhash);
}
State inspect(Settings *s,Hash version) {
    Blob m={0},j={0};
    if (!read_blob(MAIN,&m)) return BLOCKED;
    int stock=!strcmp(m.hash,STOCK),patched=!strcmp(m.hash,PATCHED);
    free(m.data);
    if (!stock && !patched) { fail("Неизвестный main. Требуется GarlicOS 1.4.9."); return BLOCKED; }
    int rec=exists(RECEIPT),bm=exists(BACK_MAIN),bj=exists(BACK_CFG);
    if (stock && (rec!=0 || bm!=0 || bj!=0)) {
        fail("Найдены прежние копии или запись установки. Проверьте CFW."); return BLOCKED;
    }
    if (patched) {
        Hash h;
        if (rec!=1) { fail("Обнаружен main v0.5 без подтверждённой пары исходных копий. Верните свои исходные файлы GarlicOS 1.4.9."); return BLOCKED; }
        if (!receipt(h) || !backups(h)) return BLOCKED;
    }
    if (!read_blob(CFG,&j)) return BLOCKED;
    int ok=settings_read(&j,s);
    memcpy(version,j.hash,65); free(j.data);
    if (!ok) return patched ? RESTORE_ONLY : BLOCKED;
    return stock ? FIRST : MANAGED;
}
static int writable(void) {
    if (access(".",W_OK) || access("skin",W_OK) || access(MAIN,W_OK) || access(CFG,W_OK))
        return fail("Для CFW и обоих файлов нужна запись.");
    return 1;
}
static int cleanup(void) {
    /* No live changes are permitted without a published plan. */
    if (!erase(TEMP)) return 0;
    for (int i=0;i<2;i++) if (!erase(before[i]) || !erase(after[i])) return 0;
    if (rmdir(WORK)) return fail("Не удалось убрать рабочий каталог (%d).",errno);
    return dir_sync(".") && checkpoint();
}
static int load_journal(const char *name,Journal *p) {
    Blob b={0};
    if (!read_blob(name,&b)) return 0;
    char tail;
    int ok=sscanf(b.data,"GMS1 %c %64s %64s %64s %64s %64s %c",
        &p->op,p->old[0],p->old[1],p->next[0],p->next[1],p->original,&tail)==6;
    free(b.data);
    ok=ok && digest_ok(p->old[1]) && digest_ok(p->next[1]) && digest_ok(p->original);
    if (ok) switch (p->op) {
    case 'I': ok=!strcmp(p->old[0],STOCK)&&!strcmp(p->next[0],PATCHED)&&!strcmp(p->old[1],p->original); break;
    case 'S': ok=!strcmp(p->old[0],PATCHED)&&!strcmp(p->next[0],PATCHED); break;
    case 'R': ok=!strcmp(p->old[0],PATCHED)&&!strcmp(p->next[0],STOCK)&&!strcmp(p->next[1],p->original); break;
    default: ok=0;
    }
    return ok || fail("Повреждён журнал. Сохраните копию CFW для проверки.");
}
static int publish(const Journal *p) {
    char text[360];
    int len=snprintf(text,sizeof text,"GMS1 %c %s %s %s %s %s\n",p->op,
                     p->old[0],p->old[1],p->next[0],p->next[1],p->original);
    Blob b={.data=text,.len=(size_t)len};
    return create(TEMP,&b,0600) && move(TEMP,PLAN);
}
static int remove_journal(const char *record) {
    return erase(record) && dir_sync(WORK) && cleanup();
}
static int rollback(const Journal *p) {
    int replace[2]={0,0};
    for (int i=0;i<2;i++) {
        Blob current={0};
        if (!read_blob(live[i],&current)) return 0;
        replace[i]=strcmp(current.hash,p->old[i])!=0;
        int known=!replace[i] || !strcmp(current.hash,p->next[i]);
        free(current.data);
        if (!known) return fail("После прерывания изменился %s. Проверьте копии.",live[i]);
        if (replace[i] && !same(before[i],p->old[i])) return 0;
    }
    for (int i=0;i<2;i++) if (replace[i] && !move(before[i],live[i])) return 0;
    for (int i=0;i<2;i++) if (!same(live[i],p->old[i])) return 0;
    if (p->op=='I') {
        /* During an exclusive install a power loss may leave an incomplete .bak.
           Remove only a byte-for-byte prefix of the verified live original. */
        for (int i=0;i<2;i++) {
            if (exists(back[i])==0) continue;
            Blob orig={0},copy={0};
            int ok=read_blob(live[i],&orig) && read_blob(back[i],&copy) &&
                   copy.len<=orig.len && !memcmp(orig.data,copy.data,copy.len);
            free(orig.data); free(copy.data);
            if (!ok) return fail("Неизвестная копия %s. Проверьте её вручную.",back[i]);
            if (!erase(back[i])) return 0;
        }
        if (!dir_sync(".") || !dir_sync("skin")) return 0;
    }
    return remove_journal(PLAN);
}
static int finish(const Journal *p) {
    for (int i=0;i<2;i++) if (!same(live[i],p->next[i])) return 0;
    if (p->op=='I') {
        if (!backups(p->original)) return 0;
        if (exists(RECEIPT)!=0) {
            Hash h; if (!receipt(h) || strcmp(h,p->original)) return fail("Чужая запись резервных копий.");
        } else {
            Blob b={.data=(char *)p->original,.len=64};
            if (!erase(TEMP) || !create(TEMP,&b,0600) || !move(TEMP,RECEIPT)) return 0;
        }
    }
    if (p->op=='R') {
        for (int i=0;i<2;i++)
            if (exists(back[i])!=0 && !same(back[i],p->next[i])) return 0;
        if (!erase(BACK_MAIN) || !erase(BACK_CFG) || !erase(RECEIPT) ||
            !dir_sync(".") || !dir_sync("skin")) return 0;
    }
    return remove_journal(DONE);
}
int recover(void) {
    if (exists(WORK)==0) return 1;
    struct stat st;
    if (lstat(WORK,&st) || !S_ISDIR(st.st_mode)) return fail("Неизвестный рабочий каталог CFW.");
    Journal p={0};
    if (exists(DONE)!=0) return load_journal(DONE,&p) && finish(&p);
    if (exists(PLAN)!=0) return load_journal(PLAN,&p) && rollback(&p);
    return cleanup();
}
static int transact(char op,const Blob old[2],const Blob next[2],const char *orig) {
    if (!writable()) return 0;
    if (mkdir(WORK,0700)) return fail("Не удалось создать каталог транзакции (%d).",errno);
    Journal p={.op=op};
    strcpy(p.original,orig);
    int ok=dir_sync(".") && checkpoint();
    for (int i=0;i<2;i++) {
        strcpy(p.old[i],old[i].hash); strcpy(p.next[i],next[i].hash);
        if (ok && strcmp(p.old[i],p.next[i]))
            ok=create(before[i],&old[i],old[i].mode) && same(before[i],p.old[i]) &&
               create(after[i],&next[i],op=='R' ? next[i].mode : old[i].mode) && same(after[i],p.next[i]);
    }
    ok=ok && same(MAIN,p.old[0]) && same(CFG,p.old[1]);
    if (ok && op=='I')
        ok=(exists(BACK_MAIN)==0 && exists(BACK_CFG)==0 && exists(RECEIPT)==0) ||
           fail("Резервная копия уже существует.");
    if (ok) ok=publish(&p);
    if (ok && op=='I')
        ok=create(BACK_MAIN,&old[0],old[0].mode) && create(BACK_CFG,&old[1],old[1].mode) &&
           dir_sync(".") && dir_sync("skin");
    if (ok) ok=backups(orig);
    for (int i=0;i<2 && ok;i++) {
        ok=same(live[i],p.old[i]);
        if (ok && strcmp(p.old[i],p.next[i])) ok=move(after[i],live[i]);
    }
    if (ok) ok=same(MAIN,p.next[0]) && same(CFG,p.next[1]) && backups(orig) && move(PLAN,DONE);
    if (ok) return finish(&p) || recover();
    char reason[sizeof problem]; strcpy(reason,problem);
    int committed=exists(DONE)==1;
    if (!recover()) return 0;
    return committed || fail("%s",reason);
}
static int change(char op,const Package *pack,const Settings *edit,const char *version) {
    Settings s; Hash current,orig;
    State state=inspect(&s,current);
    if ((op=='I' && state!=FIRST) || (op=='S' && state!=MANAGED) ||
        (op=='R' && state!=MANAGED && state!=RESTORE_ONLY)) return fail("Состояние CFW изменилось. Откройте приложение заново.");
    Blob old[2]={{0}},dst[2]={{0}};
    Blob config={0},stock={0};
    int ok=read_blob(MAIN,&old[0]) && read_blob(CFG,&old[1]);
    if (ok && strcmp(old[0].hash,op=='I' ? STOCK : PATCHED)) ok=fail("Файл main изменился.");
    if (ok && version && strcmp(version,old[1].hash)) ok=fail("Настройки изменились на карте. Откройте заново.");
    if (ok) {
        if (op=='I') strcpy(orig,old[1].hash);
        else ok=receipt(orig) && backups(orig);
    }
    if (ok && op=='R') {
        ok=read_blob(BACK_MAIN,&stock) && read_blob(BACK_CFG,&config) &&
           !strcmp(stock.hash,STOCK) && !strcmp(config.hash,orig);
        dst[0]=stock;
    } else if (ok) {
        ok=settings_write(&old[1],pack ? &pack->additions : NULL,edit,&config);
        dst[0]=pack ? pack->main : old[0];
    }
    dst[1]=config;
    if (ok) ok=transact(op,old,dst,orig);
    free(old[0].data); free(old[1].data); free(config.data); free(stock.data);
    return ok;
}
int install(const Package *p) { return change('I',p,NULL,NULL); }
int restore(void) { return change('R',NULL,NULL,NULL); }
int save(const Settings *s,const Hash version) { return change('S',NULL,s,version); }

