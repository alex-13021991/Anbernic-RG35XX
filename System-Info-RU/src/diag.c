#include "diag.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

void diag_log(const char *format, ...) {
    int saved=errno;
    char message[700],stamp[32]="--:--:--";
    time_t t=time(NULL);struct tm tm;
    if (localtime_r(&t,&tm)) strftime(stamp,sizeof(stamp),"%Y-%m-%d %H:%M:%S",&tm);
    va_list args;
    va_start(args,format);
    vsnprintf(message,sizeof(message),format,args);
    va_end(args);
    for (char *p=message;*p;p++) if (*p=='\n' || *p=='\r') *p=' ';
    fprintf(stderr,"[%s] %s\n",stamp,message);
    fflush(stderr);
    errno=saved;
}

