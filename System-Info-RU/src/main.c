#include "ui.h"
#include "screens.h"
#include "diag.h"
#include <stdio.h>
#include <string.h>

/* The worker owns pending until it publishes finished. Only the UI owns shown. */
static Report shown,pending;
static Volume volumes[2],scan_volumes[2];
static System sysdata;
static SDL_Thread *worker;
static int cancel_flag,finished,stage,ok,from_cache;
static int tab,slot,platforms,scroll,visited;
static char status[200];
static Uint32 scan_start;

static void log_volumes(void) {
    for (int i=0;i<2;i++) {
        const Volume *v=&volumes[i];
        diag_log("TF%d %s: present=%d total=%llu available=%llu card_id_readable=%d",
                 i+1,card_paths[i],v->present,(unsigned long long)v->total,
                 (unsigned long long)v->available,v->cid[0]!=0);
    }
}

static int scan_tick(void *unused) {
    (void)unused;
    return !__atomic_load_n(&cancel_flag,__ATOMIC_RELAXED);
}
static int scan_worker(void *unused) {
    (void)unused; ok=1;
    for (int i=0;i<2;i++) {
        __atomic_store_n(&stage,i,__ATOMIC_RELAXED);
        pending.present[i]=scan_volumes[i].present;
        memcpy(pending.cid[i],scan_volumes[i].cid,sizeof(pending.cid[i]));
        if (pending.present[i] &&
            !scan_card(card_paths[i],&pending.cards[i],pending.detailed,scan_tick,NULL)) {ok=0;break;}
    }
    pending.completed=time(NULL);
    __atomic_store_n(&finished,1,__ATOMIC_RELEASE);
    return 0;
}
static void start_scan(int full) {
    if (worker) return;
    read_volumes(volumes);
    log_volumes();
    if (!volumes[0].present && !volumes[1].present) {
        snprintf(status,sizeof(status),"Карты памяти недоступны");return;
    }
    if (shown.completed && !report_matches(&shown,volumes)) memset(&shown,0,sizeof(shown));
    memset(&pending,0,sizeof(pending));pending.detailed=full;
    memcpy(scan_volumes,volumes,sizeof(volumes));
    cancel_flag=finished=stage=0;
    scan_start=SDL_GetTicks();
    diag_log("Starting %s scan",full?"detailed":"quick");
    worker=SDL_CreateThread(scan_worker,NULL);
    if (!worker) {
        diag_log("SDL_CreateThread: %s",SDL_GetError());
        snprintf(status,sizeof(status),"Не удалось начать сканирование");
    }
}
static void describe_report(void) {
    unsigned errors=shown.cards[0].errors+shown.cards[1].errors;
    if (errors) {snprintf(status,sizeof(status),"Данные неполные · пропусков: %u",errors);return;}
    if (!shown.detailed) {snprintf(status,sizeof(status),"A — рассчитать размеры файлов на обеих картах");return;}
    char date[64]="";
    struct tm *tm=localtime(&shown.completed);
    if (tm) strftime(date,sizeof(date),"%d.%m.%Y %H:%M",tm);
    snprintf(status,sizeof(status),"%s%s%s",from_cache?"Сохранённый результат":"Полное сканирование",
             date[0]?" · ":"",date);
}
static void finish_scan(void) {
    SDL_WaitThread(worker,NULL);worker=NULL;
    diag_log("Scan finished: duration_ms=%u cancelled=%d completed=%d",
             (unsigned)(SDL_GetTicks()-scan_start),cancel_flag,ok);
    read_volumes(volumes);
    if (cancel_flag || !ok) {
        snprintf(status,sizeof(status),cancel_flag?"Сканирование отменено":"Сканирование прервано · A повторить");return;
    }
    if (!report_matches(&pending,volumes)) {
        diag_log("Card set changed during scan; discarding result");
        memset(&shown,0,sizeof(shown));
        snprintf(status,sizeof(status),"Карты изменились · A повторить");return;
    }
    memcpy(&shown,&pending,sizeof(shown));from_cache=0;
    for (int i=0;i<2;i++) if (shown.present[i]) {
        const Inventory *c=&shown.cards[i];const Measure *m=c->category;
        diag_log("TF%d platforms=%u games=%llu apps=%llu saves=%llu states=%llu screenshots=%llu BIOS=%llu errors=%u",
                 i+1,c->nplatforms,(unsigned long long)m[GAMES].count,(unsigned long long)m[APPS].count,
                 (unsigned long long)m[SAVES].count,(unsigned long long)m[STATES].count,
                 (unsigned long long)m[SHOTS].count,(unsigned long long)m[BIOS].count,c->errors);
    }
    if (shown.detailed) cache_save(&shown);
    describe_report();
}
static void storage_enter(void) {
    read_volumes(volumes);
    if (!visited || (shown.completed && !report_matches(&shown,volumes))) {
        visited=1;
        memset(&shown,0,sizeof(shown));
        if (cache_load(&shown,volumes)) {from_cache=1;describe_report();}
        else start_scan(0);
    }
    if (!volumes[slot].present && volumes[1-slot].present) slot=1-slot;
}
int main(void) {
    diag_log("Startup: ARMv7 / SDL 1.2 / 640x480");
    if (!ui_init("System Info RU")) return 1;
    char driver[64]={0};
    diag_log("Video: %s %dx%d %d bpp",SDL_VideoDriverName(driver,sizeof(driver))?driver:"?",
             screen->w,screen->h,screen->format->BitsPerPixel);
    int dirty=1,running=1;
    Uint32 next_system=0,next_progress=0;
    read_volumes(volumes);
    log_volumes();read_system(&sysdata);next_system=SDL_GetTicks()+1000;
    diag_log("Linux %s, CPU %s, RAM %ld KiB, battery %d%%, CPU temperature %d C (-1 = unavailable)",
             sysdata.kernel,sysdata.cpu,sysdata.ram_total,sysdata.battery,sysdata.temperature);
    while (running && !stopping) {
        Uint32 now=SDL_GetTicks();
        if (worker && __atomic_load_n(&finished,__ATOMIC_ACQUIRE)) {
            finish_scan();dirty=1;
        }
        if (!tab && (Sint32)(now-next_system)>=0) {
            read_system(&sysdata);next_system=now+1000;dirty=1;
        }
        if (worker && (Sint32)(now-next_progress)>=0) {
            snprintf(status,sizeof(status),"TF%d · %s",
                1+__atomic_load_n(&stage,__ATOMIC_RELAXED),
                cancel_flag?"Отменяем сканирование…":pending.detailed?
                "Считаем файлы и их размеры…":"Считаем игры и файлы…");
            next_progress=now+250;dirty=1;
        }
        if (dirty) {
            if (tab) draw_storage(&volumes[slot],slot,&shown,platforms,scroll,status,worker!=NULL);
            else draw_system(&sysdata,volumes);
            dirty=0;
        }
        int k=key();
        if (k==KEY_MENU || k==KEY_QUIT) running=0;
        else if (k==KEY_L || k==KEY_R) {
            if (tab!=(k==KEY_R)) diag_log("Tab: %s",k==KEY_R?"storage":"system");
            tab=k==KEY_R?1:0;
            if (tab) storage_enter(); else next_system=now;
        } else if (k==KEY_B) {
            if (tab && worker) __atomic_store_n(&cancel_flag,1,__ATOMIC_RELAXED);
            else running=0;
        } else if (tab) {
            if (k==KEY_LEFT || k==KEY_RIGHT) {
                if (slot!=(k==KEY_RIGHT)) diag_log("Selected TF%d",1+(k==KEY_RIGHT));
                slot=k==KEY_RIGHT;scroll=0;
            }
            if (k==KEY_X) {
                platforms=!platforms;scroll=0;
                diag_log("Storage view: %s",platforms?"platforms":"summary");
            }
            int max_top=(int)shown.cards[slot].nplatforms-6;
            if (platforms && k==KEY_UP && scroll>0) scroll--;
            if (platforms && k==KEY_DOWN && scroll<max_top) scroll++;
            if (k==KEY_A) start_scan(1);
        } else if (k==KEY_A) {read_volumes(volumes);log_volumes();next_system=now;}
        if (k!=KEY_NONE) dirty=1;
        SDL_Delay(20);
    }
    if (worker) {diag_log("Stopping scan on exit");__atomic_store_n(&cancel_flag,1,__ATOMIC_RELAXED);SDL_WaitThread(worker,NULL);}
    diag_log("Closing interface");
    ui_close();return 0;
}

