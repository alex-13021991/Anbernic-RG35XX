#ifndef SYSTEM_H
#define SYSTEM_H
#include "storage.h"
typedef struct {
    char kernel[96], cpu[120], uptime[64];
    int load, mhz, temperature, battery;
    long ram_total, ram_avail;
} System;
void read_system(System *s);
#endif

