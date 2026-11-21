#ifndef SCREENS_H
#define SCREENS_H
#include "system.h"
#include "catalog.h"
enum { PLATFORM_ROWS = 6 };
void draw_system(const System *s,const Volume volumes[2]);
void draw_storage(const Catalog *c,int slot,const Report *report,int by_platform,
                  int scroll,SortMode sort,const char *status,int busy);
#endif

