#ifndef CACHE_H
#define CACHE_H
#include "storage.h"
#include <time.h>
typedef struct {
    Inventory cards[2];
    char cid[2][64];
    uint32_t present[2];
    time_t completed;
    int detailed;
} Report;
int report_matches(const Report *r,const Volume v[2]);
int cache_load(Report *r,const Volume v[2]);
int cache_save(const Report *r);
#endif

