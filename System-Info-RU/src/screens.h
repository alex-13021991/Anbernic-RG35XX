#ifndef SCREENS_H
#define SCREENS_H
#include "system.h"
#include "cache.h"
void draw_system(const System *s,const Volume volumes[2]);
void draw_storage(const Volume *v,int slot,const Report *report,int by_platform,
                  int scroll,const char *status,int busy);
#endif

