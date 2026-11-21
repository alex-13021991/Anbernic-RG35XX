#include "catalog.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static int by_platform(const void *left, const void *right) {
    const Platform *a = left, *b = right;
    int cmp = strcasecmp(a->name, b->name);
    return cmp ? cmp : strcmp(a->name, b->name);
}

static int descending(uint64_t a, uint64_t b) {
    return (a < b) - (a > b);
}

static int by_count(const void *left, const void *right) {
    const Platform *a = left, *b = right;
    int cmp = descending(a->files.count, b->files.count);
    return cmp ? cmp : by_platform(a, b);
}

static int by_size(const void *left, const void *right) {
    const Platform *a = left, *b = right;
    int cmp = descending(a->files.bytes, b->files.bytes);
    return cmp ? cmp : by_platform(a, b);
}

static void add_measure(Measure *sum, Measure value) {
    sum->count += value.count;
    sum->bytes += value.bytes;
}

void catalog_build(Catalog *out, const Report *report, const Volume volumes[2],
                   int slot, SortMode order) {
    out->volume = (Volume){0};
    out->errors = out->nplatforms = 0;
    memset(out->category, 0, sizeof(out->category));
    int unknown_capacity = 0;
    for (int i = 0; i < 2; i++) {
        const Volume *v = &volumes[i];
        if ((slot != CARD_BOTH && slot != i) || !v->present) continue;
        out->volume.present = 1;
        out->volume.total += v->total;
        out->volume.available += v->available;
        if (!v->total) unknown_capacity = 1;
        if (slot == i) memcpy(out->volume.cid, v->cid, sizeof(v->cid));
        if (!report->present[i]) continue;

        const Inventory *card = &report->cards[i];
        out->errors += card->errors;
        for (unsigned k = 0; k < CATEGORY_COUNT; k++)
            add_measure(&out->category[k], card->category[k]);

        unsigned rows = card->nplatforms;
        if (rows > MAX_PLATFORMS) {
            rows = MAX_PLATFORMS;
            out->errors++;
        }
        memcpy(out->platforms + out->nplatforms, card->platforms, rows * sizeof(Platform));
        out->nplatforms += rows;
    }
    if (unknown_capacity) out->volume.total = out->volume.available = 0;
    if (out->nplatforms < 2) return;

    /* A name sort makes equal FAT folder names adjacent, even if the two
       cards use different letter case. Compact in place before sorting totals. */
    qsort(out->platforms, out->nplatforms, sizeof(Platform), by_platform);
    unsigned unique = 0;
    for (unsigned i = 0; i < out->nplatforms; i++) {
        Platform *row = &out->platforms[i];
        if (unique && !strcasecmp(out->platforms[unique - 1].name, row->name)) {
            add_measure(&out->platforms[unique - 1].files, row->files);
        } else {
            if (unique != i) out->platforms[unique] = *row;
            unique++;
        }
    }
    out->nplatforms = unique;
    if (order == SORT_COUNT || order == SORT_SIZE)
        qsort(out->platforms, unique, sizeof(Platform), order == SORT_COUNT ? by_count : by_size);
}
