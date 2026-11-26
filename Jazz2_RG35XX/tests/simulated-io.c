// SPDX-License-Identifier: GPL-3.0-or-later
// Только для QEMU. Файловый framebuffer и FIFO создаёт smoke.py.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

static int matches(int fd, const char* path) {
    struct stat a,b;
    return path && fstat(fd,&a)==0 && stat(path,&b)==0 && a.st_dev==b.st_dev && a.st_ino==b.st_ino;
}
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args,request);
    void* arg=va_arg(args,void*);
    va_end(args);
    if ((request==FBIOGET_FSCREENINFO || request==FBIOGET_VSCREENINFO ||
         request==FBIOPAN_DISPLAY || request==FBIO_WAITFORVSYNC) && matches(fd,getenv("TEST_FB"))) {
        const int bpp = getenv("TEST_BPP") ? atoi(getenv("TEST_BPP")) : 16;
        const int padded = getenv("TEST_PADDED") != NULL;
        const int stride = (640 + (padded ? 16 : 0)) * (bpp/8);
        if (request==FBIOGET_FSCREENINFO) {
            struct fb_fix_screeninfo* f=arg;
            memset(f,0,sizeof(*f)); f->line_length=stride; f->smem_len=stride*(480+(padded ? 4 : 0));
            f->type=FB_TYPE_PACKED_PIXELS; f->visual=FB_VISUAL_TRUECOLOR;
        } else if (request==FBIOGET_VSCREENINFO) {
            struct fb_var_screeninfo* v=arg;
            memset(v,0,sizeof(*v)); v->bits_per_pixel=bpp;
            v->xres=v->xres_virtual=640; v->yres=v->yres_virtual=480;
            v->red.offset=11; v->red.length=5; v->green.offset=5; v->green.length=6; v->blue.length=5;
            if (bpp == 32) {
                v->red.offset=16; v->green.offset=8; v->blue.offset=0;
                v->red.length=v->green.length=v->blue.length=8;
                v->transp.offset=24; v->transp.length=8;
                if (getenv("TEST_GARLIC_RGB")) {
                    v->red.offset=0; v->blue.offset=16; v->transp.length=0;
                }
            }
            if (padded) { v->xoffset=8; v->yoffset=2; v->xres_virtual+=16; v->yres_virtual+=4; }
        } else { errno=ENOTTY; return -1; }
        return 0;
    }
    if (_IOC_TYPE(request)=='E' && _IOC_NR(request)==0x06 && matches(fd,getenv("TEST_PAD"))) {
        const char* name="RG35XX Gamepad";
        size_t size=_IOC_SIZE(request);
        if (size < strlen(name)+1) { errno=EINVAL; return -1; }
        memcpy(arg,name,strlen(name)+1);
        return strlen(name)+1;
    }
    static int (*real_ioctl)(int,unsigned long,...);
    if (!real_ioctl) real_ioctl=dlsym(RTLD_NEXT,"ioctl");
    return real_ioctl(fd,request,arg);
}
