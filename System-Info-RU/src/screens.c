#include "ui.h"
#include "screens.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define PANEL 0x12161c
#define BORDER 0x485765
#define SELECTED 0x305265
#define TRACK 0x26313a
#define METER 0x7999aa

static void heading(int tab) {
    page("SYSTEM INFO RU","RG35XX");
    const char *names[]={"Система","Хранилище"};
    for (int i=0;i<2;i++) {
        int x=18+i*305;
        box(x,63,298,35,i==tab?SELECTED:PANEL);
        text(x+10,70,i?"R1":"L1",small,C_ACCENT,34);
        center(x,70,298,names[i],small,i==tab?C_TEXT:C_MUTED);
    }
}
static void meter(int x,int y,int w,int percent) {
    box(x,y,w,5,TRACK);
    if (percent>0 && percent<=100) box(x,y,w*percent/100,5,METER);
}
static void reading(char *out,size_t cap,int n,const char *unit) {
    if (n<0) snprintf(out,cap,"—"); else snprintf(out,cap,"%d %s",n,unit);
}
static void volume_text(char *dst,size_t len,const Volume *v) {
    if (!v->present) {snprintf(dst,len,"Карта отсутствует");return;}
    if (!v->total) {snprintf(dst,len,"Объём недоступен");return;}
    char used[48],all[48];
    format_bytes(used,sizeof(used),v->available);format_bytes(all,sizeof(all),v->total);
    snprintf(dst,len,"%s из %s свободно",used,all);
}
static const char *platform_label(const char *name,char *dst,size_t len) {
    static const struct {const char *folder,*system;} known[]={
        {"FC","NES"},{"SFC","Super Nintendo"},{"MD","Mega Drive"},{"GG","Game Gear"},
        {"PS","PlayStation"},{"PS1","PlayStation"},{"GB","Game Boy"},{"GBC","Game Boy Color"},
        {"GBA","Game Boy Advance"},{"PCE","PC Engine"},{"SMS","Master System"},
        {"MS","Master System"},{"VB","Virtual Boy"},{"WS","WonderSwan"},
        {"WSC","WonderSwan Color"},{"NGP","Neo Geo Pocket"},{"NGC","Neo Geo Pocket Color"},
        {"PORTS","Порты"}
    };
    for (size_t i=0;i<sizeof(known)/sizeof(known[0]);i++) if (!strcasecmp(name,known[i].folder)) {
        snprintf(dst,len,"%s · %s",name,known[i].system);return dst;
    }
    return name;
}
void draw_system(const System *s,const Volume volumes[2]) {
    char line[240],val[64],freq[64];
    heading(0);
    text(18,110,"ПРОЦЕССОР",small,C_ACCENT,290);
    text(333,110,"ОПЕРАТИВНАЯ ПАМЯТЬ",small,C_ACCENT,289);
    box(18,138,292,99,PANEL); box(330,138,292,99,PANEL);
    reading(freq,sizeof(freq),s->mhz,"МГц"); reading(val,sizeof(val),s->load,"%");
    text(30,148,s->cpu,font,C_TEXT,268);
    snprintf(line,sizeof(line),"%s   ·   %s",freq,val);
    text(30,185,line,small,C_MUTED,268);
    meter(30,219,268,s->load);
    int ram=-1;
    if (s->ram_total>0 && s->ram_avail>=0) {
        long used=s->ram_total-s->ram_avail;
        snprintf(line,sizeof(line),"%ld из %ld МиБ занято",used/1024,s->ram_total/1024);
        ram=(int)(100*used/s->ram_total);
        snprintf(val,sizeof(val),"%ld МиБ доступно",s->ram_avail/1024);
    } else {snprintf(line,sizeof(line),"—");snprintf(val,sizeof(val),"Нет показаний");}
    text(342,148,line,font,C_TEXT,268);text(342,185,val,small,C_MUTED,268);
    meter(342,219,268,ram);
    text(18,248,"Ядро Linux",small,C_MUTED,110); text(132,246,s->kernel,font,C_TEXT,180);
    text(330,248,"Время работы",small,C_MUTED,126); text(461,246,s->uptime,font,C_TEXT,160);
    text(18,282,"Аккумулятор",small,C_MUTED,118);
    reading(val,sizeof(val),s->battery,"%");text(144,280,val,font,C_TEXT,140);
    text(330,282,"Темп. CPU",small,C_MUTED,126);
    reading(val,sizeof(val),s->temperature,"°C");text(461,280,val,font,C_TEXT,160);
    box(18,316,604,1,BORDER);
    for (int i=0;i<2;i++) {
        int y=330+38*i;
        text(18,y,i?"TF2":"TF1",font,C_ACCENT,52);
        volume_text(line,sizeof(line),volumes+i);
        text(88,y,line,font,C_TEXT,524);
    }
    text(18,416,"Для оригинальной RG35XX с GarlicOS 1.4.9",small,C_MUTED,604);
    footer("L1/R1 Вкладки      A Обновить      B Выход");
    flip();
}
void draw_storage(const Catalog *c,int slot,const Report *r,int platforms,int top,
                  SortMode sort,const char *status,int busy) {
    static const char *const card_names[CARD_VIEW_COUNT]={"TF1","TF2","Обе карты"};
    static const int card_widths[CARD_VIEW_COUNT]={60,60,112};
    const char *const names[]={"Игры · всего","Приложения","Сохранения игр","Состояния эмулятора","Скриншоты","Файлы BIOS"};
    char buf[200];
    heading(1);
    int x=18;
    for (int i=0;i<CARD_VIEW_COUNT;i++) {
        box(x,109,card_widths[i],31,i==slot?SELECTED:PANEL);
        center(x,114,card_widths[i],card_names[i],small,C_TEXT);
        x+=card_widths[i]+8;
    }
    if (!c->volume.present) {
        center(18,226,604,slot==CARD_BOTH?"Карты памяти недоступны":"Карта отсутствует",font,C_TEXT);
        if (slot!=CARD_BOTH) center(18,266,604,"← →  Выбрать другую карту",small,C_MUTED);
    } else {
        volume_text(buf,sizeof(buf),&c->volume);
        text(x+6,114,buf,small,C_TEXT,622-(x+6));
        const char *first="СВОДКА",*count="Количество",*size="Размер";
        if (platforms) {
            first=sort==SORT_PLATFORM?"ПЛАТФОРМЫ ↑":"ПЛАТФОРМЫ";
            if (sort==SORT_COUNT) count="Количество ↓";
            if (sort==SORT_SIZE) size="Размер ↓";
        }
        text(22,156,first,small,C_ACCENT,299);
        text(333,156,count,small,C_ACCENT,141);
        text(486,156,size,small,C_ACCENT,126);
        box(18,182,604,1,BORDER);
        int rows=platforms?(int)c->nplatforms:CATEGORY_COUNT;
        if (top>rows-PLATFORM_ROWS) top=rows-PLATFORM_ROWS;
        if (top<0) top=0;
        if (!r->completed) center(18,246,604,busy?"Подсчитываем файлы…":"Статистика ещё не собрана",font,C_TEXT);
        else if (!rows) center(18,246,604,"Платформ с играми не найдено",font,C_TEXT);
        else for (int i=top;i<rows && i<top+PLATFORM_ROWS;i++) {
            const Measure *m=platforms?&c->platforms[i].files:&c->category[i];
            int y=187+35*(i-top);
            if ((i-top)%2==0) box(18,y,604,34,PANEL);
            text(24,y+4,platforms?platform_label(c->platforms[i].name,buf,sizeof(buf)):names[i],font,C_TEXT,298);
            snprintf(buf,sizeof(buf),"%llu",(unsigned long long)m->count);
            text(336,y+4,buf,font,C_TEXT,134);
            if (r->detailed) format_bytes(buf,sizeof(buf),m->bytes);
            else snprintf(buf,sizeof(buf),"—");
            text(486,y+4,buf,font,C_TEXT,131);
        }
        if (platforms && rows>PLATFORM_ROWS) {
            box(620,187,3,209,TRACK);
            box(620,187+top*209/rows,3,PLATFORM_ROWS*209/rows+1,METER);
        }
    }
    text(18,411,status,small,C_MUTED,604);
    if (platforms) {
        footer(busy?"←→ Карта   ↑↓ Список   X Сводка   Y Сортировка   B Отмена":
                    "←→ Карта   ↑↓ Список   X Сводка   Y Сортировка   A Скан   B Выход");
    } else {
        footer(busy?"←→ Карта   X Платформы   B Отмена":
                    "←→ Карта   X Платформы   A Сканировать   B Выход");
    }
    flip();
}

