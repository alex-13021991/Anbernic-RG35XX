#ifndef PREVIEW_BROWSER_H
#define PREVIEW_BROWSER_H
#include <limits.h>
typedef struct { char *name; int folder; } Entry;
typedef struct {
    const char *root[2];
    char folder[PATH_MAX];
    Entry *entries;
    int card,count,row;
} Browser;
int browser_enter(Browser *b,const char *folder,const char *select_name);
int browser_back(Browser *b);
int browser_path(const Browser *b,char out[PATH_MAX]);
void browser_clear(Browser *b);
#endif
