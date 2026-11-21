#include "ui.h"
#include "screens.h"
#include "catalog.h"
#include <stdio.h>
#include <string.h>

/* The worker owns pending until it publishes finished. Only the UI owns shown. */
static Report shown,pending;
static Volume volumes[2],scan_volumes[2];
static System sysdata;
static SDL_Thread *worker;
static int cancel_flag,finished,stage,ok,from_cache;
static int tab,selection=CARD_TF1,platforms,scroll,visited;
static SortMode sort=SORT_PLATFORM;
static Catalog catalog;
static int catalog_dirty=1;
static char status[200];

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
    catalog_dirty=1;
    if (shown.completed && !report_matches(&shown,volumes)) memset(&shown,0,sizeof(shown));
    if (!volumes[0].present && !volumes[1].present) {
        snprintf(status,sizeof(status),"Карты памяти недоступны");return;
    }
    memset(&pending,0,sizeof(pending));pending.detailed=full;
    memcpy(scan_volumes,volumes,sizeof(volumes));
    cancel_flag=finished=stage=0;
    worker=SDL_CreateThread(scan_worker,NULL);
    if (!worker) {
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
    read_volumes(volumes);
    catalog_dirty=1;
    if (shown.completed && !report_matches(&shown,volumes)) memset(&shown,0,sizeof(shown));
    if (cancel_flag || !ok) {
        snprintf(status,sizeof(status),cancel_flag?"Сканирование отменено":"Сканирование прервано · A повторить");return;
    }
    if (!report_matches(&pending,volumes)) {
        memset(&shown,0,sizeof(shown));
        snprintf(status,sizeof(status),"Карты изменились · A повторить");return;
    }
    memcpy(&shown,&pending,sizeof(shown));from_cache=0;
    if (shown.detailed) cache_save(&shown);
    describe_report();
}
static void storage_enter(void) {
    read_volumes(volumes);
    catalog_dirty=1;
    if (!visited || (shown.completed && !report_matches(&shown,volumes))) {
        visited=1;
        memset(&shown,0,sizeof(shown));
        if (cache_load(&shown,volumes)) {from_cache=1;describe_report();}
        else start_scan(0);
    }
    if (selection!=CARD_BOTH && !volumes[selection].present && volumes[1-selection].present)
        selection=1-selection;
}
int main(void) {
    if (!ui_init("System Info RU")) return 1;
    int dirty=1,running=1;
    Uint32 next_system=0,next_progress=0;
    read_volumes(volumes);
    read_system(&sysdata);next_system=SDL_GetTicks()+1000;
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
            if (tab) {
                if (catalog_dirty) {
                    catalog_build(&catalog,&shown,volumes,selection,sort);
                    int last=catalog.nplatforms>PLATFORM_ROWS?(int)catalog.nplatforms-PLATFORM_ROWS:0;
                    if (scroll>last) scroll=last;
                    catalog_dirty=0;
                }
                draw_storage(&catalog,selection,&shown,platforms,scroll,sort,status,worker!=NULL);
            }
            else draw_system(&sysdata,volumes);
            dirty=0;
        }
        int k=key();
        if (k==KEY_MENU || k==KEY_QUIT) running=0;
        else if (k==KEY_L || k==KEY_R) {
            tab=k==KEY_R?1:0;
            if (tab) storage_enter(); else next_system=now;
        } else if (k==KEY_B) {
            if (tab && worker) __atomic_store_n(&cancel_flag,1,__ATOMIC_RELAXED);
            else running=0;
        } else if (tab) {
            if (k==KEY_LEFT || k==KEY_RIGHT) {
                int direction=k==KEY_RIGHT?1:-1;
                selection=(selection+direction+CARD_VIEW_COUNT)%CARD_VIEW_COUNT;
                scroll=0;catalog_dirty=1;
            }
            if (k==KEY_X) {
                platforms=!platforms;scroll=0;
            }
            if (platforms && k==KEY_Y) {
                sort=(SortMode)((sort+1)%SORT_MODE_COUNT);
                scroll=0;catalog_dirty=1;
            }
            int max_top=(int)catalog.nplatforms-PLATFORM_ROWS;
            if (platforms && k==KEY_UP && scroll>0) scroll--;
            if (platforms && k==KEY_DOWN && scroll<max_top) scroll++;
            if (k==KEY_A) start_scan(1);
        } else if (k==KEY_A) {read_volumes(volumes);next_system=now;}
        if (k!=KEY_NONE) dirty=1;
        SDL_Delay(20);
    }
    if (worker) {__atomic_store_n(&cancel_flag,1,__ATOMIC_RELAXED);SDL_WaitThread(worker,NULL);}
    ui_close();return 0;
}

