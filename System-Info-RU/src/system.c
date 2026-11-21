#include "system.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>

const char *const card_paths[2]={"/mnt/mmc","/mnt/SDCARD"};

static int line_from(const char *path,char *out,size_t len) {
    FILE *f=fopen(path,"r");
    if (!f) return 0;
    int ok=fgets(out,len,f)!=NULL;
    fclose(f);
    if (ok) out[strcspn(out,"\r\n")]=0;
    return ok;
}
static long number(const char *path) {
    char line[128],*end;
    if (!line_from(path,line,sizeof(line))) return -1;
    long value=strtol(line,&end,10);
    return end==line?-1:value;
}
void read_volumes(Volume cards[2]) {
    struct stat parent,seen[2];
    memset(cards,0,2*sizeof(*cards));
    if (stat("/mnt",&parent)!=0) return;
    for (int i=0;i<2;i++) {
        if (stat(card_paths[i],&seen[i]) || !S_ISDIR(seen[i].st_mode) ||
            seen[i].st_dev==parent.st_dev ||
            (i && cards[0].present && seen[0].st_dev==seen[1].st_dev)) continue;
        cards[i].present=1;
        struct statvfs v;
        if (!statvfs(card_paths[i],&v)) {
            uint64_t unit=v.f_frsize?v.f_frsize:v.f_bsize;
            cards[i].total=v.f_blocks*unit;
            cards[i].available=v.f_bavail*unit;
        }
        char path[160];
        snprintf(path,sizeof(path),"/sys/dev/block/%u:%u/../device/cid",
                 (unsigned)major(seen[i].st_dev),(unsigned)minor(seen[i].st_dev));
        line_from(path,cards[i].cid,sizeof(cards[i].cid));
        if (strlen(cards[i].cid)!=32 || strspn(cards[i].cid,"0123456789abcdefABCDEF")!=32)
            cards[i].cid[0]=0;
    }
}
static int cpu_load(void) {
    static unsigned long long prev_total,prev_idle;
    unsigned long long t[8]={0};
    char buf[512];
    if (!line_from("/proc/stat",buf,sizeof(buf))) return -1;
    int n=sscanf(buf,"cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                 t,t+1,t+2,t+3,t+4,t+5,t+6,t+7);
    if (n<4) return -1;
    unsigned long long total=0,idle=t[3]+t[4];
    for (int i=0;i<n;i++) total+=t[i];
    int value=-1;
    if (prev_total && total>prev_total && idle>=prev_idle && idle-prev_idle<=total-prev_total)
        value=(int)(100*(total-prev_total-(idle-prev_idle))/(total-prev_total));
    prev_total=total; prev_idle=idle;
    return value;
}
static void memory(System *s) {
    FILE *f=fopen("/proc/meminfo","r");
    s->ram_total=s->ram_avail=-1;
    if (!f) return;
    long free_kb=-1,buffers=0,cached=0;
    char buf[200],key[64]; long value;
    while (fgets(buf,sizeof(buf),f)) {
        if (sscanf(buf,"%63[^:]: %ld",key,&value)!=2) continue;
        if (!strcmp(key,"MemTotal")) s->ram_total=value;
        else if (!strcmp(key,"MemAvailable")) s->ram_avail=value;
        else if (!strcmp(key,"MemFree")) free_kb=value;
        else if (!strcmp(key,"Buffers")) buffers=value;
        else if (!strcmp(key,"Cached")) cached=value;
    }
    fclose(f);
    if (s->ram_avail<0 && free_kb>=0) s->ram_avail=free_kb+buffers+cached;
    if (s->ram_avail>s->ram_total) s->ram_avail=s->ram_total;
}
static void cpu_name(char *dst,size_t cap) {
    FILE *f=fopen("/proc/cpuinfo","r");
    snprintf(dst,cap,"—");
    if (!f) return;
    char line[256];
    while (fgets(line,sizeof(line),f)) {
        char *colon=strchr(line,':'); if (!colon) continue;
        *colon=0;
        char *value=colon+1;
        while (isspace((unsigned char)*value)) value++;
        value[strcspn(value,"\r\n")]=0;
        if (!strncasecmp(line,"CPU part",8) && strtoul(value,NULL,0)==0xc09) {
            snprintf(dst,cap,"ARM Cortex-A9"); break;
        }
        if ((!strncmp(line,"model name",10) || !strncmp(line,"Processor",9)) && *value)
            snprintf(dst,cap,"%s",value);
    }
    fclose(f);
}
static int cpu_temp(void) {
    for (int i=0;i<16;i++) {
        char path[120],kind[128];
        snprintf(path,sizeof(path),"/sys/class/thermal/thermal_zone%d/type",i);
        if (!line_from(path,kind,sizeof(kind))) continue;
        for (char *p=kind;*p;p++) *p=tolower((unsigned char)*p);
        if (!strstr(kind,"cpu") && !strstr(kind,"soc") && !strstr(kind,"owl")) continue;
        snprintf(path,sizeof(path),"/sys/class/thermal/thermal_zone%d/temp",i);
        long v=number(path);
        if (v>=0 && v<=150000) return v>150?(int)(v/1000):(int)v;
    }
    return -1;
}
void read_system(System *s) {
    if (!s->kernel[0]) {
        struct utsname u;
        snprintf(s->kernel,sizeof(s->kernel),"%s",uname(&u)?"—":u.release);
        cpu_name(s->cpu,sizeof(s->cpu));
    }
    s->load=cpu_load();
    long freq=number("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (freq<=0) freq=number("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq");
    s->mhz=freq>0?(int)(freq/1000):-1;
    s->temperature=cpu_temp();
    s->battery=(int)number("/sys/class/power_supply/battery/capacity");
    if (s->battery>100) s->battery=-1;
    long seconds=number("/proc/uptime");
    if (seconds>=0) snprintf(s->uptime,sizeof(s->uptime),"%ld ч %02ld мин",seconds/3600,(seconds/60)%60);
    else snprintf(s->uptime,sizeof(s->uptime),"—");
    memory(s);
}

