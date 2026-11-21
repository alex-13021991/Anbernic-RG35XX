#include "storage.h"
#include "cache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static Report r,copy;
static Volume volumes[2];
static int stop(void *p) {return --*(int *)p>0;}
static void matches(Measure m,uint64_t n,uint64_t bytes) {assert(m.count==n);assert(m.bytes==bytes);}
int main(void) {
    /* For a native Linux run, start in the two-card fixture directory. */
#ifdef __wasi__
    assert(chdir("/fixtures")==0);
#endif
    FILE *expected=fopen("expected","r");
    unsigned long long disc_bytes; assert(expected && fscanf(expected,"%llu",&disc_bytes)==1);fclose(expected);
    for (int full=0;full<=1;full++) {
        Inventory *one=&r.cards[0],*two=&r.cards[1];
        assert(scan_card("TF1",one,full,NULL,NULL) && scan_card("TF2",two,full,NULL,NULL));
        assert(!one->errors && !two->errors);
        assert(one->nplatforms==3 && two->nplatforms==2);
        assert(!strcmp(one->platforms[0].name,"FC") && !strcmp(one->platforms[2].name,"PS"));
        matches(one->category[GAMES],5,full?disc_bytes+12+9:0);
        matches(one->platforms[2].files,3,full?disc_bytes:0);
        matches(one->category[APPS],1,full?19:0);
        matches(one->category[SAVES],2,full?9:0);
        matches(one->category[STATES],1,full?5:0);
        matches(one->category[SHOTS],3,full?13:0);
        matches(one->category[BIOS],1,full?4:0);
        matches(two->category[GAMES],2,full?14:0);
        matches(two->category[SAVES],1,full?4:0);
        matches(two->category[BIOS],1,full?5:0);
    }
    int n=7;
    assert(!scan_card("TF1",&copy.cards[0],1,stop,&n));
    assert(scan_card("empty",&copy.cards[0],1,NULL,NULL));
    assert(!copy.cards[0].errors && !copy.cards[0].nplatforms);
    r.present[0]=r.present[1]=volumes[0].present=volumes[1].present=1;
    strcpy(r.cid[0],"first");strcpy(volumes[0].cid,"first");
    strcpy(r.cid[1],"second");strcpy(volumes[1].cid,"second");
    r.completed=1234;r.detailed=1;
    assert(cache_save(&r) && cache_load(&copy,volumes) && !memcmp(&r,&copy,sizeof(r)));
    volumes[1].present=0;assert(!cache_load(&copy,volumes));
    volumes[1].present=1;strcpy(volumes[1].cid,"replaced");assert(!cache_load(&copy,volumes));
    strcpy(volumes[1].cid,"second");
    FILE *f=fopen("inventory.cache","r+b");assert(f);assert(!fseek(f,123,SEEK_SET));
    int b=fgetc(f);assert(b!=EOF);assert(!fseek(f,123,SEEK_SET));fputc(b^1,f);fclose(f);
    assert(!cache_load(&copy,volumes));unlink("inventory.cache");
    r.cards[0].errors=1;assert(!cache_save(&r));
    printf("PASS: quick and detailed scans; both cards; multidisc/track grouping; >4 GiB; "
           "apps; screenshots, BIOS, saves and states; cancellation; empty card; "
           "cache round-trip, corruption, card swap and partial result rejection.\n");
    return 0;
}

