#include "ui.h"
#include "daemon.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>

static const SDL_Color white = {238, 245, 250, 0}, pale = {173, 191, 204, 0};
static const SDL_Color mint = {114, 232, 182, 0}, ice = {125, 208, 245, 0};
static const SDL_Color warning = {248, 180, 112, 0};

static void draw(int state, const char *error) {
    box(0, 0, 640, 480, 0x101923);
    text(26, 17, "Screenshot Manager", title, white, 582);
    box(26, 60, 588, 2, 0x30414f);
    box(26, 80, 588, 98, state == 1 ? 0x16372f : 0x243341);
    box(26, 80, 5, 98, state == 1 ? 0x64dca8 : 0x7995a9);
    text(48, 94, "Служба скриншотов", small, pale, 400);
    text(48, 122, state == 1 ? "Включена" : state == 0 ? "Выключена" : "Статус неизвестен",
         title, state == 1 ? mint : white, 450);
    box(514, 113, 70, 29, state == 1 ? 0x307e63 : 0x445767);
    box(state == 1 ? 557 : 518, 117, 23, 21, state == 1 ? 0x89f0c1 : 0xb4c4cf);
    box(27, 201, 53, 36, 0x324b60);
    center(27, 206, 53, "L2", font, white);
    center(81, 206, 20, "+", font, pale);
    box(102, 201, 53, 36, 0x324b60);
    center(102, 206, 53, "R2", font, white);
    text(170, 207, "Сделать снимок экрана", font, white, 427);
    text(28, 253, "Папка снимков", small, pale, 188);
    text(28, 277, "TF1  /  Screenshots", font, ice, 576);
    box(26, 318, 588, 1, 0x30414f);
    text(28, 334, state == 1 ? "Можно выйти из меню и делать снимки."
                             : "Включи службу для съёмки в приложениях.", font, white, 582);
    text(28, 365, "После перезагрузки включи её снова.", font, pale, 582);
    if (error) text(28, 400, error, small, warning, 582);
    box(0, 438, 640, 42, 0x20303e);
    box(28, 447, 25, 25, 0x39657c);
    center(28, 449, 25, "A", small, white);
    text(63, 447, state == 1 ? "Выключить" : state == 0 ? "Включить" : "Проверить", font, white, 310);
    box(490, 447, 25, 25, 0x394958);
    center(490, 449, 25, "B", small, white);
    text(525, 447, "Выход", font, white, 90);
    flip();
}

int main(void) {
    int lock = open("/tmp/rg35xx-screenshot-manager.lock", O_CREAT | O_RDWR, 0600);
    if (lock < 0 || flock(lock, LOCK_EX | LOCK_NB) < 0) return 1;
    if (!ui_init("Screenshot Manager")) return 1;
    /* The input adapter belongs only to the menu. The daemon needs raw input. */
    unsetenv("LD_PRELOAD");
    SDL_EnableKeyRepeat(0, 0);
    int state = daemon_status();
    const char *error = NULL;
    draw(state, NULL);
    Uint32 opened = SDL_GetTicks(), last = opened;
    for (;;) {
        int k = key();
        if (k == KEY_B || k == KEY_MENU || k == KEY_QUIT) break;
        Uint32 now = SDL_GetTicks();
        if (k == KEY_A && now - opened >= 350) {
            int live = daemon_status();
            error = NULL;
            if (live != state || live < 0) {
                if (live < 0) error = "Не удалось прочитать список процессов.";
            } else {
                int target = !live;
                int rc = target ? daemon_start() : daemon_stop();
                for (int i = 0; !rc && i < 24; ++i) {
                    SDL_Delay(40);
                    if (daemon_status() == target) break;
                }
                if (target && !rc) SDL_Delay(180);
                live = daemon_status();
                if (rc || live != target)
                    error = target ? "Не удалось запустить screenshot."
                                   : "Не удалось остановить службу.";
            }
            state = live;
            draw(state, error);
            last = SDL_GetTicks();
        } else if (now - last >= 800) {
            int live = daemon_status();
            if (live != state) { state = live; error = NULL; draw(state, NULL); }
            last = now;
        }
        SDL_Delay(24);
    }
    ui_close();
    close(lock);
    return 0;
}
