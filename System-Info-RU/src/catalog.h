#ifndef CATALOG_H
#define CATALOG_H

#include "cache.h"

enum { CARD_TF1, CARD_TF2, CARD_BOTH, CARD_VIEW_COUNT };
typedef enum { SORT_PLATFORM, SORT_COUNT, SORT_SIZE, SORT_MODE_COUNT } SortMode;

/* Unrelated platforms on two full cards can occupy every row. */
typedef struct {
    Volume volume;
    Measure category[CATEGORY_COUNT];
    Platform platforms[2 * MAX_PLATFORMS];
    unsigned nplatforms, errors;
} Catalog;

void catalog_build(Catalog *out, const Report *report, const Volume volumes[2],
                   int slot, SortMode order);

#endif
