#ifndef GMS_H
#define GMS_H
#include <stddef.h>
#include <sys/types.h>
#include "yyjson.h"

#define STOCK "4b3192302227875074f15d6e706abf19721aaebae90c8a9e5b37ecfb3b069157"
#define PATCHED "b20853ebb449a85e7f27562d5d2f945a6bbcb3c3127dfa762ffc8e785e6eb988"
#define ADDITIONS "229f596ea8ed07cddd522c0caa47df0e83e52764acc847f6878b783932c4bbbb"
#define MAIN "main"
#define CFG "skin/settings.json"
#define BACK_MAIN MAIN ".bak"
#define BACK_CFG CFG ".bak"
#define FILE_LIMIT (4u*1024u*1024u)
typedef char Hash[65];
typedef struct { char *data; size_t len; mode_t mode; Hash hash; } Blob;
typedef enum { CENTER, LEFT, RIGHT } Alignment; /* Garlic main's enum */
typedef struct { Alignment align; int margin,width,scroll,speed,delay,pause; } Settings;
typedef struct { int x, width, offset; } Row;
typedef enum { BLOCKED, FIRST, MANAGED, RESTORE_ONLY } State;
typedef struct { Blob main, additions; } Package;
extern char problem[512];
int fail(const char *fmt, ...);
int read_blob(const char *path, Blob *b);
void fingerprint(Blob *b);
int exists(const char *path); /* 0 absent, 1 present, -1 error */
int load_package(Package *p);
int enter_system(const char *cfw);
int recover(void);
State inspect(Settings *s, Hash version);
int install(const Package *p);
int save(const Settings *s, const Hash version);
int restore(void);
yyjson_doc *json_object(const Blob *b);
int settings_read(const Blob *b, Settings *s);
int settings_write(const Blob *old,const Blob *add,const Settings *edit,Blob *out);
Settings defaults(void);
Row layout(Settings s,int cover,int width,int selected,unsigned elapsed);
#endif

