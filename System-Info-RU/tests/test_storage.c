#include "storage.h"
#include "cache.h"
#include "catalog.h"
#include "photo_report.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

static Report r,copy;
static Volume volumes[2];
static int stop(void *p) {return --*(int *)p>0;}
static void matches(Measure m,uint64_t n,uint64_t bytes) {assert(m.count==n);assert(m.bytes==bytes);}

static Catalog catalog;

static void expect_names(const char *const names[],unsigned count) {
    assert(catalog.nplatforms==count);
    for (unsigned i=0;i<count;i++) assert(!strcasecmp(catalog.platforms[i].name,names[i]));
}
static void check_catalog(void) {
    Volume v[2]={{1,64,30,"first"},{1,32,18,"second"}};
    photo_report(&r,v);
    r.cards[0].errors=3;
    r.cards[1].errors=1;
    memcpy(&copy,&r,sizeof(r));
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_PLATFORM);
    assert(catalog.volume.present && catalog.volume.total==96 && catalog.volume.available==48);
    assert(catalog.errors==4);
    const unsigned combined[]={1237,4,17,133,13,1382};
    for (int k=0;k<CATEGORY_COUNT;k++) {
        matches(catalog.category[k],combined[k],r.cards[0].category[k].bytes+r.cards[1].category[k].bytes);
    }
    const char *const alpha[]={"FC","FDS","MD","PORTS","PS","SFC"};
    expect_names(alpha,6);
    matches(catalog.platforms[0].files,808,186ULL*1048576);
    matches(catalog.platforms[2].files,346,500ULL*1048576);
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_COUNT);
    const char *const by_count[]={"FC","MD","SFC","PS","FDS","PORTS"};
    expect_names(by_count,6);
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_SIZE);
    const char *const by_size[]={"PS","PORTS","MD","FC","SFC","FDS"};
    expect_names(by_size,6);
    catalog_build(&catalog,&r,v,CARD_TF1,SORT_COUNT);
    const char *const tf1_sort[]={"FC","MD","SFC","PS","FDS"};
    expect_names(tf1_sort,5);
    assert(catalog.category[GAMES].count==282 && catalog.volume.total==64);
    catalog_build(&catalog,&r,v,CARD_TF2,SORT_SIZE);
    const char *const tf2_sort[]={"PORTS","MD","FC"};
    expect_names(tf2_sort,3);
    assert(catalog.category[GAMES].count==955 && catalog.volume.total==32);
    assert(!memcmp(&r,&copy,sizeof(r)));  /* Views must never mutate the cache payload. */

    v[1].present=0;
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_COUNT);
    expect_names(tf1_sort,5);
    assert(catalog.category[GAMES].count==282 && catalog.errors==3 && catalog.volume.total==64);
    v[0].present=0;
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_PLATFORM);
    assert(!catalog.volume.present && !catalog.nplatforms && !catalog.category[GAMES].count);
    v[1].present=1;
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_SIZE);
    expect_names(tf2_sort,3);
    assert(catalog.volume.total==32 && catalog.errors==1);
    v[0].present=1;
    v[0].total=0;
    catalog_build(&catalog,&r,v,CARD_BOTH,SORT_PLATFORM);
    assert(catalog.volume.present && !catalog.volume.total && catalog.category[GAMES].count==1237);

    /* The merged list can be larger than the per-card cache layout.
       Equal counts/sizes break ties alphabetically for both numeric sorts. */
    memset(&r,0,sizeof(r));
    for (int slot=0;slot<2;slot++) {
        r.present[slot]=1;
        Inventory *c=&r.cards[slot];
        c->nplatforms=MAX_PLATFORMS;
        for (unsigned i=0;i<MAX_PLATFORMS;i++) {
            Platform *p=&c->platforms[i];
            snprintf(p->name,sizeof(p->name),"%c-%03u",'B'-slot,MAX_PLATFORMS-1-i);
            p->files=(Measure){1,UINT64_C(1)<<34};
        }
    }
    for (SortMode mode=SORT_PLATFORM;mode<SORT_MODE_COUNT;mode++) {
        catalog_build(&catalog,&r,v,CARD_BOTH,mode);
        assert(catalog.nplatforms==2*MAX_PLATFORMS);
        for (unsigned i=1;i<catalog.nplatforms;i++)
            assert(strcasecmp(catalog.platforms[i-1].name,catalog.platforms[i].name)<0);
    }
    puts("PASS: photo totals 1237 games / FC 808; sort columns, ties and 512 rows; "
         "single/absent cards, category sums, capacity and cache immutability.");
}
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
    check_catalog();
    return 0;
}

