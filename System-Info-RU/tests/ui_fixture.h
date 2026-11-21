/* Single-threaded SDL stand-in for navigation and layout tests. */
#ifndef RG_UI_H
#define RG_UI_H
#include <stdint.h>
typedef int sig_atomic_t;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef int SDL_Color;
typedef int TTF_Font;
typedef struct { unsigned char BitsPerPixel; } FixtureFormat;
typedef struct { int w,h; FixtureFormat *format; } FixtureScreen;
typedef struct { int (*fn)(void *); void *arg; int age; } SDL_Thread;
extern FixtureScreen *screen;
extern volatile sig_atomic_t stopping;
extern TTF_Font *font,*small,*title;
extern const SDL_Color C_TEXT,C_MUTED,C_ACCENT;
enum { KEY_NONE,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_A,KEY_B,KEY_X,KEY_Y,KEY_L,KEY_R,KEY_MENU,KEY_QUIT };
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32);
SDL_Thread *SDL_CreateThread(int (*fn)(void *),void *arg);
void SDL_WaitThread(SDL_Thread *,int *);
const char *SDL_GetError(void);
char *SDL_VideoDriverName(char *,int);
int ui_init(const char *);
void ui_close(void);
int key(void);
void page(const char *,const char *);
void box(int,int,int,int,unsigned);
void text(int,int,const char *,TTF_Font *,SDL_Color,int);
void center(int,int,int,const char *,TTF_Font *,SDL_Color);
void footer(const char *);
void flip(void);
#endif

