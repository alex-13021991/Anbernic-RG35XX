#include "ui.h"
#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <errno.h>
static UsbChoice saved,choice;
static char live_mtp[40],live_adb[40],live_host[40],cable[100];
static int selected, applied;
static int exists(const char *p) { struct stat s; return stat(p,&s)==0 && S_ISREG(s.st_mode); }
static UsbChoice read_saved(void) {
    UsbChoice s={exists("/misc/enableMTP"),exists("/misc/enableADB"),exists("/misc/waitForUSB")}; return s;
}
static int read_line(const char *p,char *out,size_t len) {
    FILE *f=fopen(p,"r"); if(!f) return 0;
    int ok=fgets(out,(int)len,f)!=NULL; fclose(f);
    if(ok) out[strcspn(out,"\r\n")]=0;
    return ok;
}
static int process_running(const char *name) {
    DIR *d=opendir("/proc"); if(!d) return -1;
    struct dirent *e; char p[300],s[128]; int result=0;
    while((e=readdir(d))) {
        if(e->d_name[0]<'0'||e->d_name[0]>'9') continue;
        snprintf(p,sizeof(p),"/proc/%s/comm",e->d_name);
        if(read_line(p,s,sizeof(s)) && !strcmp(s,name)) {result=1;break;}
    }
    closedir(d); return result;
}
/* Linux power_supply is the physical VBUS signal; 0 means the cable is gone. */
static int usb_power_online(void) {
    DIR *d=opendir("/sys/class/power_supply"); if(!d) return -1;
    struct dirent *e; char p[350],type[40],online[16]; int seen=0;
    while((e=readdir(d))) {
        if(e->d_name[0]=='.') continue;
        snprintf(p,sizeof(p),"/sys/class/power_supply/%s/type",e->d_name);
        int is_usb=read_line(p,type,sizeof(type)) && !strncmp(type,"USB",3);
        if(!is_usb && strncmp(e->d_name,"usb",3)) continue;
        snprintf(p,sizeof(p),"/sys/class/power_supply/%s/online",e->d_name);
        if(!read_line(p,online,sizeof(online))) continue;
        seen=1;
        if(!strcmp(online,"1")) { closedir(d); return 1; }
    }
    closedir(d); return seen?0:-1;
}
static int connection_state(const char *s) {
    if(!strcmp(s,"DISCONNECTED") || !strcmp(s,"NOT ATTACHED") || !strcmp(s,"not attached")) return 1;
    if(!strcmp(s,"CONFIGURED") || !strcmp(s,"configured")) return 3;
    if(!strcmp(s,"CONNECTED") || !strcmp(s,"connected") || !strcmp(s,"ATTACHED") ||
       !strcmp(s,"attached") || !strcmp(s,"POWERED") || !strcmp(s,"powered") ||
       !strcmp(s,"DEFAULT") || !strcmp(s,"default") || !strcmp(s,"ADDRESSED") ||
       !strcmp(s,"addressed") || !strcmp(s,"SUSPENDED") || !strcmp(s,"suspended")) return 2;
    return 0;
}
static int gadget_state(void) {
    DIR *d=opendir("/sys/class/udc"); struct dirent *e; char p[350],s[64]; int state=0;
    if(d) {
        while((e=readdir(d))) {
            if(e->d_name[0]=='.') continue;
            snprintf(p,sizeof(p),"/sys/class/udc/%s/state",e->d_name);
            if(read_line(p,s,sizeof(s))) {
                int value=connection_state(s);
                if(value==1) { state=1; break; }
                if(value>state) state=value;
            }
        }
        closedir(d);
        if(state) return state;
    }
    return read_line("/sys/class/android_usb/android0/state",s,sizeof(s))?connection_state(s):0;
}
static int usb_role(void) {
    DIR *d=opendir("/sys/class/usb_role"); struct dirent *e; char p[350],role[32]; int result=-1;
    if(!d) return -1;
    while((e=readdir(d))) {
        if(e->d_name[0]=='.') continue;
        snprintf(p,sizeof(p),"/sys/class/usb_role/%s/role",e->d_name);
        if(read_line(p,role,sizeof(role))) {
            if(!strcmp(role,"host")) { result=1; break; }
            if(!strcmp(role,"none")) result=0;
            else if(!strcmp(role,"device") && result<0) result=2;
        }
    }
    closedir(d); return result;
}
static int function_has(const char *s,const char *token) {
    size_t n=strlen(token);
    while(*s) { while(*s==' '||*s==',') ++s; const char *e=s; while(*e && *e!=',' && *e!=' ') ++e;
        if((size_t)(e-s)==n && !strncmp(s,token,n)) return 1; s=e;
    } return 0;
}
static void status(void) {
    char enabled[32],functions[128]; int role;
    strcpy(live_mtp,"неизвестно"); strcpy(live_adb,"неизвестно"); strcpy(live_host,"неизвестно");
    strcpy(cable,"USB-подключение: неизвестно");
    if(read_line("/sys/class/android_usb/android0/enable",enabled,sizeof(enabled))) {
        if(!strcmp(enabled,"0")) { strcpy(live_mtp,"выкл."); strcpy(live_adb,"выкл."); }
        else if(!strcmp(enabled,"1") && read_line("/sys/class/android_usb/android0/functions",functions,sizeof(functions))) {
            int m=process_running("mtp-server"),a=process_running("adbd");
            strcpy(live_mtp,function_has(functions,"mtp")?(m==1?"вкл.":m==0?"нет службы":"неизвестно"):"выкл.");
            strcpy(live_adb,function_has(functions,"adb")?(a==1?"вкл.":a==0?"нет службы":"неизвестно"):"выкл.");
            strcpy(live_host,"выкл.");
        }
    }
    /* Do not infer active host mode from waitForUSB, a next-boot flag. */
    role=usb_role();
    if(role==1) {strcpy(live_host,"вкл.");strcpy(live_mtp,"выкл.");strcpy(live_adb,"выкл.");}
    else if(role==0||role==2) strcpy(live_host,"выкл.");

    /* android0/state may stay CONFIGURED after VBUS disappears. Prefer the
       physical USB power signal, then the UDC state, then the legacy node. */
    int power=usb_power_online(),link=gadget_state();
    if(role==1) strcpy(cable,"USB: режим хоста активен");
    else if(power==0||(power<0&&role==0)) strcpy(cable,"USB: кабель не подключён");
    else if(link==3) strcpy(cable,"USB: соединение с ПК настроено");
    else if(link==2) strcpy(cable,"USB: кабель подключён");
    else if(power==1) strcpy(cable,"USB: кабель есть, ПК не определён");
    else if(link==1) strcpy(cable,"USB: ПК не подключён");
    else strcpy(cable,"USB: статус не определён");
}
static void draw(void) {
    char line[180]; page("USB МЕНЕДЖЕР",NULL);
    text(18,63,"ТЕКУЩИЙ СТАТУС",small,C_ACCENT,604);
    snprintf(line,sizeof(line),"MTP: %s    ADB: %s",live_mtp,live_adb);
    text(18,89,line,font,C_TEXT,604);
    snprintf(line,sizeof(line),"USB-ввод: %s",live_host); text(18,116,line,small,C_TEXT,604);
    text(292,119,cable,small,C_MUTED,330);
    text(18,151,"НАСТРОЙКИ СЛЕДУЮЩЕЙ ЗАГРУЗКИ",small,C_ACCENT,604);
    const char *labels[]={"MTP — передача файлов","ADB — отладка","USB-ввод — геймпады"};
    int checked[]={choice.mtp,choice.adb,choice.host};
    for(int i=0;i<3;i++) {
        int y=177+i*43; if(selected==i) box(16,y,608,39,0x305265);
        outline(28,y+9,21,21,selected==i?0xc0cbd4:0x485765);
        if(checked[i]) box(33,y+14,11,11,selected==i?0xc0cbd4:0x9aa6b0);
        text(62,y+6,labels[i],font,C_TEXT,546);
    }
    text(18,314,"MTP и ADB могут работать вместе.",small,C_MUTED,604);
    text(18,337,"USB-ввод работает только отдельно.",small,C_MUTED,604);
    const char *note=!usb_equal(saved,choice)?"Есть несохранённые изменения":applied?"Сохранено. Новый режим включится после перезагрузки.":"Галочки — настройки следующей загрузки.";
    text(18,366,note,small,C_ACCENT,604);
    button(18,396,170,"Применить",selected==3,0);
    button(200,396,248,"Перезагрузить",selected==4,0);
    button(460,396,162,"Выход",selected==5,0);
    footer("↑↓ Выбор   A Подтвердить   B Выход"); flip();
}
static int run_apply(void) {
    char m[2]={'0'+choice.mtp,0},a[2]={'0'+choice.adb,0},h[2]={'0'+choice.host,0};
    pid_t p=fork(); if(p<0) return 14;
    if(!p) { execl("/bin/sh","sh","./apply-usb.sh",m,a,h,(char*)NULL); _exit(127); }
    int s; while(waitpid(p,&s,0)<0) { if(errno!=EINTR) return 14; }
    return WIFEXITED(s)?WEXITSTATUS(s):14;
}
static void apply(void) {
    if(!usb_valid(choice)) {message("Несовместимые режимы","USB-ввод должен работать отдельно.","Снимите лишние галочки.");return;}
    if(usb_equal(saved,choice) && access("/misc/.usb-manager-backup",F_OK)!=0) {
        message("Настройки не изменены","Перезагрузка — отдельной кнопкой.","Текущий статус показан вверху."); return;
    }
    box(14,361,612,27,0x12161c); text(18,366,"Сохранение...",small,C_ACCENT,604); flip();
    int rc=run_apply(); saved=read_saved();
    if(rc==0 && usb_equal(saved,choice)) {
        applied=1; message("Настройки сохранены","Для применения нужна перезагрузка.","Можно выйти и перезагрузить позже.");
    } else if(rc==11) message("MTP не установлен","Установите MTP-комплект и запуск usb.sh.","Подробности — в инструкции к архиву MTP.");
    else if(rc==12) message("Настройки заняты","Другая операция уже выполняется.","Повторите позже; при сбое перезагрузите.");
    else if(rc==13) message("Ошибка записи","Не удалось открыть MISC для записи.","Настройки не изменены. См. app.log.");
    else if(rc==16) message("Ошибка защиты MISC","Не удалось вернуть режим только чтения.","Проверьте app.log; перезагрузите консоль.");
    else if(rc==15) message("Ошибка восстановления","Проверьте MISC и app.log.","Не перезагружайте до проверки настроек.");
    else if(rc==17) {choice=saved;message("Восстановлены настройки","Предыдущее сохранение было прервано.","Выберите режимы и примените заново.");}
    else message("Сохранение не выполнено","Проверьте карту и журнал app.log.","Перечитаны фактически сохранённые флаги.");
    fprintf(stderr,"Apply exit=%d; saved=%d%d%d\n",rc,saved.mtp,saved.adb,saved.host);
}
static int exit_allowed(void) {
    return usb_equal(saved,choice)||confirm("Выйти без сохранения?","Выбранные изменения будут потеряны.","Сохранённые настройки останутся.","Выйти");
}
int main(void) {
    if(!ui_init("USB Manager")) return 1;
    saved=choice=read_saved(); status(); Uint32 last=SDL_GetTicks(); int done=0,dirty=1;
    while(!done && !stopping) {
        if(SDL_GetTicks()-last>=1500) { status();last=SDL_GetTicks();dirty=1; }
        if(dirty){draw();dirty=0;} int k=key();if(k!=KEY_NONE)dirty=1;
        if(k==KEY_UP) selected=(selected+5)%6;
        if(k==KEY_DOWN) selected=(selected+1)%6;
        if(k==KEY_LEFT && selected>=3) selected=selected==3?5:selected-1;
        if(k==KEY_RIGHT && selected>=3) selected=selected==5?3:selected+1;
        if(k==KEY_QUIT) done=1;
        if(k==KEY_B || k==KEY_MENU) done=exit_allowed();
        if(k==KEY_A) {
            if(selected<3) usb_toggle(&choice,selected);
            else if(selected==3) apply();
            else if(selected==5) done=exit_allowed();
            else if(access("/misc/.usb-manager-backup/ready",F_OK)==0) message("Запись была прервана","Сначала нажмите «Применить» для отката.","Проверьте восстановленные настройки.");
            else if(!usb_equal(saved,choice)) message("Есть изменения","Сначала нажмите «Применить».","Перезагрузка не сохраняет галочки.");
            else if(confirm("Перезагрузить консоль?","Завершите передачу файлов на ПК.","Будут применены сохранённые настройки.","Перезагрузить")) {
                sync();
                if(reboot(RB_AUTOBOOT)<0) {
                    char detail[100]; snprintf(detail,sizeof(detail),"%s",strerror(errno));
                    message("Не удалось перезагрузить",detail,"Проверьте права и перезагрузите вручную.");
                } else done=1;
            }
        }
        SDL_Delay(25);
    }
    ui_close(); return 0;
}
