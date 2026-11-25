#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { FILES,EDIT,TOOLS,ALIGN,CROP,PREFS,PREVIEW } Page;
enum { ALIGN_TOOL,CROP_TOOL,PREVIEW_TOOL,STEP_TOOL,PREFS_TOOL,SAVE_TOOL,RESTORE_TOOL,TOOLS_COUNT };
typedef struct {
    Browser b;
    Theme theme;
    Image img;
    History history;
    Placement saved,draft;
    Digest version;
    Prefs prefs,proposed;
    char path[PATH_MAX];
    Page page;
    int running,tool,cell,edge,option,step;
} App;
static const char *positions[]={"Слева сверху","Сверху по центру","Справа сверху",
    "Слева по центру","По центру","Справа по центру","Слева снизу","Снизу по центру","Справа снизу"};
static const char *edges[]={"Левый край","Верхний край","Правый край","Нижний край"};
static Placement current(const App *a) { return a->history.list[a->history.at]; }
static int dirty(const App *a) { return !same_place(current(a),a->saved); }
static void popup(App *a,const char *heading,const char *line) {
    header(heading,"Preview Editor");
    label(24,180,594,font,INK,line);
    footer("A / B   Вернуться","");
    SDL_Flip(screen);
    for (;;) {
        Key k=read_key();
        if (k==QUIT) a->running=0;
        if (k==A || k==B || k==QUIT) return;
    }
}
static int confirm(App *a,const char *heading,const char *line,const char *verb) {
    int yes=0;
    for (;;) {
        header(heading,"Preview Editor");
        label(24,143,592,font,INK,line);
        label(24,181,592,small,MUTED,a->b.entries[a->b.row].name);
        box(24,271,181,45,yes ? PANEL : SELECTED);
        box(217,271,385,45,yes ? SELECTED : PANEL);
        label(40,280,150,font,INK,"Отмена"); label(233,280,350,font,INK,verb);
        footer("← →   Выбор       A   Подтвердить       B   Отмена","");
        SDL_Flip(screen);
        Key k=read_key();
        if (k==LEFT || k==RIGHT) yes=!yes;
        if (k==QUIT) a->running=0;
        if (k==A || k==B || k==QUIT) return k==A && yes;
    }
}
static void show_files(App *a) {
    Browser *b=&a->b;
    char status[96],location[PATH_MAX+32];
    if (!*b->folder) {
        header("Открыть PNG","Preview Editor");
        label(24,53,600,small,MUTED,"Карты памяти");
        list_row(0,b->row==0,"TF1   Системная карта",access(b->root[0],R_OK) ? "Недоступна" : "");
        list_row(1,b->row==1,"TF2   Вторая карта",access(b->root[1],R_OK) ? "Недоступна" : "");
        footer("↑ ↓   Карта       A   Открыть       B   Выход","");
        return;
    }
    snprintf(status,sizeof status,"%d / %d",b->count ? b->row+1 : 0,b->count);
    header("Открыть PNG",status);
    snprintf(location,sizeof location,"TF%d   %s",b->card+1,b->folder+strlen(b->root[b->card]));
    label(23,53,601,small,MUTED,location);
    int start=clamp(b->row-4,0,b->count>9 ? b->count-9 : 0);
    for (int i=start;i<b->count && i<start+9;i++)
        list_row(i-start,b->row==i,b->entries[i].name,b->entries[i].folder ? "Папка" : "PNG");
    if (!b->count) label(24,122,595,font,INK,"В этой папке нет PNG и подпапок.");
    footer("↑ ↓   Выбор       L / R   Страница       A   Открыть       B   Назад",
        "X   Восстановить выбранный PNG из резервной копии");
}
static void show_tools(App *a) {
    const char *names[]={"Автоматическое выравнивание по краю","Обрезка","Предпросмотр GarlicOS",
        "Шаг перемещения","Отступ и панели темы","Сохранить PNG","Восстановить оригинал"};
    char step[32]; snprintf(step,sizeof step,"%d px",a->step);
    header("Инструменты",dirty(a) ? "Есть изменения" : "");
    label(24,53,600,small,MUTED,a->b.entries[a->b.row].name);
    for (int i=0;i<TOOLS_COUNT;i++)
        list_row(i,a->tool==i,names[i],i==STEP_TOOL ? step : i==SAVE_TOOL ? "RGBA  640 × 480" : "");
    footer("↑ ↓   Выбор       A   Открыть       B   К холсту","");
}
static void show_prefs(App *a) {
    const char *names[]={"Отступ от края","Верхняя строка","Нижняя строка"};
    int values[]={a->proposed.margin,a->proposed.top,a->proposed.bottom};
    header("Отступ и панели темы","");
    label(24,53,600,small,MUTED,"Размеры в пикселях холста 640 × 480");
    for (int i=0;i<3;i++) {
        char v[64];
        if (values[i]<0) snprintf(v,sizeof v,"Авто · %d px",i==1 ? a->theme.top : a->theme.bottom);
        else snprintf(v,sizeof v,"%d px",values[i]);
        list_row(i,a->option==i,names[i],v);
    }
    footer("↑ ↓   Строка       ← →   ±1       L / R   ±8       X   По умолчанию",
        "A   Сохранить настройки       B   Отмена");
}
static void show_edit(App *a) {
    Placement p=a->page==EDIT ? current(a) : a->draft;
    char status[96],line[160];
    snprintf(status,sizeof status,"%d%%    X %d    Y %d",p.percent,p.x,p.y);
    if (a->page==ALIGN) {
        header(positions[a->cell],"");
        for (int i=0;i<9;i++)
            box(579+i%3*14,5+i/3*12,11,9,i==a->cell ? ACCENT : LINE);
        int top,bottom;
        theme_panels(&a->theme,a->prefs,&top,&bottom);
        snprintf(line,sizeof line,"Отступ %d px     Верх %d px     Низ %d px     Масштаб %d%%",a->prefs.margin,top,bottom,p.percent);
        footer("Крестовина   Выбрать позицию       A   Применить       B   Отмена",line);
    } else if (a->page==CROP) {
        snprintf(line,sizeof line,"Обрезка · %s",edges[a->edge]);
        header(line,status);
        footer("↑ ↓   Край       ←   Вернуть       →   Срезать       L / R   ±8 px",
            "A   Применить       B   Отмена");
    } else {
        header(a->b.entries[a->b.row].name,status);
        snprintf(line,sizeof line,"Крестовина   Шаг %d px       L / R   Масштаб       A   Инструменты",a->step);
        footer(line,"X   Отменить правку       Y   Повторить       B   К файлам");
    }
    draw_art(&a->img,p,&a->theme,a->prefs,0,&a->b);
}
static void draw(App *a) {
    switch (a->page) {
    case FILES: show_files(a); break;
    case TOOLS: show_tools(a); break;
    case PREFS: show_prefs(a); break;
    case PREVIEW: draw_art(&a->img,current(a),&a->theme,a->prefs,1,&a->b); break;
    default: show_edit(a); break;
    }
    SDL_Flip(screen);
}
static void close_doc(App *a) {
    free_image(&a->img);
    a->page=FILES;
}
static void restore(App *a,const char *path) {
    if (!has_original(path)) { popup(a,"Восстановление","Для этого PNG ещё нет резервной копии."); return; }
    if (!confirm(a,"Восстановить оригинал?","Текущая версия PNG будет заменена исходной.","Восстановить")) return;
    if (!restore_document(path)) { popup(a,"Ошибка восстановления",problem); return; }
    close_doc(a);
    popup(a,"Оригинал восстановлен","На карте снова исходный PNG. Резервная копия сохранена.");
}
static void open_doc(App *a,const char *path) {
    Image im={0};
    if (!load_document(path,&im,a->version)) { popup(a,"Не удалось открыть PNG",problem); return; }
    Placement p;
    if (!initial(&im,&p)) {
        free_image(&im); popup(a,"Пустое изображение","Все пиксели PNG полностью прозрачны."); return;
    }
    a->img=im; a->saved=p; a->history=(History){0}; a->history.list[0]=p;
    strcpy(a->path,path); a->tool=0; a->page=EDIT;
}
static void file_key(App *a,Key k) {
    Browser *b=&a->b;
    if (!*b->folder) {
        if (k==B) a->running=0;
        if (k==UP || k==DOWN) b->row=1-b->row;
        if (k==A) {
            b->card=b->row;
            if (!browser_enter(b,b->root[b->card],"Roms")) popup(a,"Карта недоступна",problem);
        }
        return;
    }
    if (k==B) {
        if (!browser_back(b)) popup(a,"Ошибка каталога",problem);
        return;
    }
    if (!b->count) return;
    if (k==UP || k==DOWN || k==L || k==R)
        b->row=clamp(b->row+(k==UP ? -1 : k==DOWN ? 1 : k==L ? -9 : 9),0,b->count-1);
    if (k!=A && k!=X) return;
    char path[PATH_MAX];
    if (!browser_path(b,path)) { popup(a,"Ошибка пути",problem); return; }
    if (b->entries[b->row].folder) {
        if (k==A && !browser_enter(b,path,NULL)) popup(a,"Ошибка каталога",problem);
    } else if (k==X) restore(a,path);
    else open_doc(a,path);
}
static void edit_key(App *a,Key k) {
    Placement p=current(a);
    switch (k) {
    case A: case MENU: a->page=TOOLS; return;
    case B:
        if (!dirty(a) || confirm(a,"Закрыть без сохранения?","На холсте есть несохранённые изменения.","Закрыть файл")) close_doc(a);
        return;
    case X: history_move(&a->history,-1); return;
    case Y: history_move(&a->history,1); return;
    case L: case R: scale(&p,p.percent+(k==L ? -5 : 5)); break;
    case UP: case DOWN: shift(&p,0,k==UP ? -a->step : a->step); break;
    case LEFT: case RIGHT: shift(&p,k==LEFT ? -a->step : a->step,0); break;
    default: return;
    }
    history_push(&a->history,p);
}
static int snap(App *a,int cell) {
    int top,bottom;
    theme_panels(&a->theme,a->prefs,&top,&bottom);
    Placement next=current(a);
    if (!align_image(&a->img,&next,cell,a->prefs.margin,top,bottom)) {
        popup(a,"Пустое выделение","Увеличьте масштаб или расширьте область обрезки.");
        return 0;
    }
    a->cell=cell; a->draft=next;
    return 1;
}
static void save(App *a) {
    render(&a->img,current(a),canvas);
    int i=0;
    while (i<CW*CH && !canvas[i].a) i++;
    if (i==CW*CH) { popup(a,"Пустой холст","Верните видимые пиксели в границы холста."); return; }
    if (!confirm(a,"Сохранить PNG 640 × 480?","Редактор сохранит оригинал перед первой заменой.","Сохранить PNG")) return;
    if (!save_document(a->path,canvas,a->version)) { popup(a,"Ошибка сохранения",problem); return; }
    a->saved=current(a); a->page=EDIT;
    popup(a,"PNG сохранён","Файл на карте проверен. Можно продолжать редактирование.");
}
static void tools_key(App *a,Key k) {
    if (k==B || k==MENU) { a->page=EDIT; return; }
    if (k==UP || k==DOWN) a->tool=(a->tool+(k==UP ? TOOLS_COUNT-1 : 1))%TOOLS_COUNT;
    if (k!=A && !(a->tool==STEP_TOOL && (k==LEFT || k==RIGHT))) return;
    switch (a->tool) {
    case ALIGN_TOOL: if (snap(a,a->cell)) a->page=ALIGN; break;
    case CROP_TOOL: a->draft=current(a); a->edge=0; a->page=CROP; break;
    case PREVIEW_TOOL: a->page=PREVIEW; break;
    case STEP_TOOL: a->step=a->step==1 ? 8 : 1; break;
    case PREFS_TOOL: a->proposed=a->prefs; a->option=0; a->page=PREFS; break;
    case SAVE_TOOL: save(a); break;
    case RESTORE_TOOL: restore(a,a->path); break;
    }
}
static void draft_key(App *a,Key k) {
    if (k==B) { a->page=TOOLS; return; }
    if (k==A) {
        history_push(&a->history,a->draft); a->page=EDIT; return;
    }
    if (a->page==ALIGN) {
        int x=a->cell%3,y=a->cell/3;
        x=clamp(x+(k==RIGHT)-(k==LEFT),0,2);
        y=clamp(y+(k==DOWN)-(k==UP),0,2);
        if (y*3+x!=a->cell) snap(a,y*3+x);
    } else {
        if (k==UP || k==DOWN) a->edge=(a->edge+(k==UP ? 3 : 1))%4;
        if (k==LEFT || k==RIGHT || k==L || k==R)
            trim(&a->draft,&a->img,a->edge,k==LEFT ? -1 : k==RIGHT ? 1 : k==L ? -8 : 8);
    }
}
static void prefs_key(App *a,Key k) {
    if (k==B) { a->page=TOOLS; return; }
    if (k==A) {
        if (save_prefs(a->proposed)) { a->prefs=a->proposed; a->page=TOOLS; }
        else popup(a,"Ошибка записи настроек",problem);
        return;
    }
    if (k==UP || k==DOWN) a->option=(a->option+(k==UP ? 2 : 1))%3;
    int *v=a->option==0 ? &a->proposed.margin : a->option==1 ? &a->proposed.top : &a->proposed.bottom;
    if (k==X) *v=a->option ? -1 : 16;
    if (k==LEFT || k==RIGHT || k==L || k==R) {
        if (*v<0) *v=a->option==1 ? a->theme.top : a->theme.bottom;
        *v=clamp(*v+(k==LEFT ? -1 : k==RIGHT ? 1 : k==L ? -8 : 8),0,120);
    }
}
int main(void) {
    if (!ui_init()) { fprintf(stderr,"Preview Editor: %s\n",SDL_GetError()); ui_close(); return 1; }
    App a={0};
    a.b.root[0]=getenv("PREVIEW_TF1"); a.b.root[1]=getenv("PREVIEW_TF2");
    if (!a.b.root[0]) a.b.root[0]="/mnt/mmc";
    if (!a.b.root[1]) a.b.root[1]="/mnt/SDCARD";
    a.running=1; a.cell=4; a.step=1; a.prefs=load_prefs();
    theme_load(&a.theme,a.b.root[0]);
    while (a.running) {
        draw(&a);
        Key k=read_key();
        if (k==QUIT) break;
        switch (a.page) {
        case FILES: file_key(&a,k); break;
        case EDIT: edit_key(&a,k); break;
        case TOOLS: tools_key(&a,k); break;
        case ALIGN: case CROP: draft_key(&a,k); break;
        case PREFS: prefs_key(&a,k); break;
        case PREVIEW: if (k==B || k==A || k==Y) a.page=TOOLS; break;
        }
    }
    free_image(&a.img); browser_clear(&a.b); theme_free(&a.theme); ui_close();
    return 0;
}
