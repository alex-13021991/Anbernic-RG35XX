#ifndef PHOTO_REPORT_H
#define PHOTO_REPORT_H
#include "cache.h"
#include <string.h>

/* Counts transcribed from the user's two cards. Sizes below are in MiB and
   serve as order test inputs; the photos only give rounded sizes. */
static void photo_report(Report *r, const Volume volumes[2]) {
    const Platform tf1[] = {{"FC",{153,54}},{"FDS",{15,4}},{"MD",{50,89}},
                            {"PS",{17,9114}},{"SFC",{47,87}}};
    const Platform tf2[] = {{"fc",{655,132}},{"md",{296,411}},{"PORTS",{4,1434}}};
    const unsigned counts[][CATEGORY_COUNT] = {
        {282,4,17,129,12,691}, {955,0,0,4,1,691}
    };
    memset(r,0,sizeof(*r));
    r->completed=1795187580;
    r->detailed=1;
    for (int slot=0;slot<2;slot++) {
        r->present[slot]=volumes[slot].present;
        memcpy(r->cid[slot],volumes[slot].cid,sizeof(r->cid[slot]));
        Inventory *c=&r->cards[slot];
        const Platform *platforms=slot?tf2:tf1;
        c->nplatforms=slot?sizeof(tf2)/sizeof(tf2[0]):sizeof(tf1)/sizeof(tf1[0]);
        for (unsigned i=0;i<c->nplatforms;i++) {
            c->platforms[i]=platforms[i];
            c->platforms[i].files.bytes*=1048576;
            c->category[GAMES].bytes+=c->platforms[i].files.bytes;
        }
        for (int cat=0;cat<CATEGORY_COUNT;cat++) c->category[cat].count=counts[slot][cat];
    }
}
#endif
