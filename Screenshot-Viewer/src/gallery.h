#ifndef RG_GALLERY_H
#define RG_GALLERY_H
#include <sys/stat.h>
#include <stddef.h>
#define MAX_SHOTS 5000
typedef struct { char *path; int card; struct stat stamp; } Shot;
typedef struct { Shot *shots; size_t count; int limited,errors; } Gallery;
int gallery_scan(Gallery *g,const char *card1,const char *card2,const char *extra);
void gallery_free(Gallery *g);
/* Return 0 only if exactly the catalogued regular file was removed. */
int gallery_delete(Gallery *g,size_t index);
int png_dimensions(const char *path,int *w,int *h);
const char *shot_name(const Shot *s);
#endif
