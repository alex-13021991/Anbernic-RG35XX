#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
char *fixture_realpath(const char *path,char *out) {
    struct stat st;
    if(!path || path[0]!='/' || strstr(path,"/../") || strstr(path,"/./")) {errno=EINVAL;return 0;}
    if(stat(path,&st)<0)return 0;
    strcpy(out,path);return out;
}
