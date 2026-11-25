#ifndef PREVIEW_EDITOR_H
#define PREVIEW_EDITOR_H
#include <stdint.h>
#include <stdio.h>

#define CW 640
#define CH 480
#define HISTORY 128
typedef struct { uint8_t r,g,b,a; } Pixel;
typedef struct { int w,h; Pixel *p; } Image;
typedef struct { int x,y,w,h; } Rect;
typedef struct { Rect crop; int x,y,percent; } Placement;
typedef struct { Placement list[HISTORY]; int at,last; } History;
typedef struct { int margin,top,bottom; } Prefs;
typedef uint8_t Digest[32];

int clamp(int value,int low,int high);
int read_png(FILE *f,Image *out);
int write_png(FILE *f,const Image *img);
void free_image(Image *im);
int output_size(int pixels,int percent);
int initial(const Image *img,Placement *out);
int visible_bounds(const Image *img,Placement p,Rect *bounds);
void shift(Placement *p,int dx,int dy);
void scale(Placement *p,int percent);
void trim(Placement *p,const Image *img,int edge,int amount);
int align_image(const Image *img,Placement *p,int cell,int margin,int top,int bottom);
void render(const Image *img,Placement p,Pixel *canvas);
void history_push(History *h,Placement next);
int history_move(History *h,int step);
int same_place(Placement a,Placement b);

extern char problem[256];
int fail(const char *s);
int load_document(const char *path,Image *out,Digest version);
int save_document(const char *path,const Pixel *canvas,Digest version);
int restore_document(const char *path);
int has_original(const char *path);
Prefs load_prefs(void);
int save_prefs(Prefs p);
#endif
