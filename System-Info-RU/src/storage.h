#ifndef STORAGE_H
#define STORAGE_H
#include <stdint.h>
#include <stddef.h>

enum { GAMES, APPS, SAVES, STATES, SHOTS, BIOS, CATEGORY_COUNT };
enum { MAX_PLATFORMS = 256 };
typedef struct { uint64_t count, bytes; } Measure;
typedef struct { char name[256]; Measure files; } Platform;
typedef struct {
    Measure category[CATEGORY_COUNT];
    Platform platforms[MAX_PLATFORMS];
    unsigned nplatforms, errors;
} Inventory;
typedef struct {
    int present;
    uint64_t total, available;
    char cid[64];
} Volume;

typedef int (*ScanTick)(void *context);
int scan_card(const char *root, Inventory *out, int sizes, ScanTick tick, void *context);
extern const char *const card_paths[2];
void read_volumes(Volume cards[2]);
void format_bytes(char *dst, size_t cap, uint64_t n);
#endif

