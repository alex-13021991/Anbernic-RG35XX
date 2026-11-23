#ifndef RG_UI_H
#define RG_UI_H
#include <SDL.h>
#include <SDL_ttf.h>
#include <signal.h>
enum { KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_A, KEY_B,
       KEY_X, KEY_Y, KEY_L, KEY_R, KEY_MENU, KEY_QUIT };
extern SDL_Surface *screen;
extern TTF_Font *font, *small, *title;
extern volatile sig_atomic_t stopping;
extern const SDL_Color C_TEXT, C_MUTED, C_ACCENT, C_DARK, C_RED;
int ui_init(const char *caption);
void ui_close(void);
void box(int x, int y, int w, int h, unsigned rgb);
void text(int x, int y, const char *s, TTF_Font *f, SDL_Color color, int maxw);
void center(int x, int y, int w, const char *s, TTF_Font *f, SDL_Color color);
void page(const char *heading, const char *right);
void footer(const char *hint);
int key(void);
void flip(void);
#endif
