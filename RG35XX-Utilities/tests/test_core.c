#include "../src/core.h"
#include "../src/gallery.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
int main(void) {
    setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);
    int cases=0;
    for(int m=0;m<2;m++)for(int a=0;a<2;a++)for(int h=0;h<2;h++) {
        UsbChoice s={m,a,h};
        if(!usb_valid(s))continue;
        for(int k=0;k<3;k++) {UsbChoice t=s;usb_toggle(&t,k);assert(usb_valid(t));cases++;}
    }
    UsbChoice s={0,0,0};usb_toggle(&s,0);usb_toggle(&s,1);assert(s.mtp && s.adb && !s.host);
    usb_toggle(&s,2);assert(!s.mtp&&!s.adb&&s.host);usb_toggle(&s,1);assert(!s.mtp&&s.adb&&!s.host);
    assert(utf8_previous("ЯЮ",4)==2);assert(utf8_previous("ЯЮ",2)==0);
    assert(grid_move(0,0,1,0)==0);assert(grid_move(0,7,0,-1)==0);
    assert(grid_move(5,7,0,1)==6);assert(grid_move(6,7,1,0)==6);
    Gallery g;
    assert(gallery_scan(&g,"/fixtures/tf1","/fixtures/tf2","/fixtures/extra.cfg"));
    printf("scan count=%u errors=%d\n",(unsigned)g.count,g.errors);
    assert(g.count==5);
    int on1=0,on2=0,dup=0;size_t del=0,changed=0;
    for(size_t i=0;i<g.count;i++) {
        on1+=g.shots[i].card==1;on2+=g.shots[i].card==2;
        if(!strcmp(shot_name(&g.shots[i]),"same.png")){dup++;if(g.shots[i].card==1)del=i;}
        if(!strcmp(shot_name(&g.shots[i]),"changed.png"))changed=i;
    }
    assert(on1==3 && on2==2 && dup==2);
    int w,h;assert(png_dimensions("/fixtures/tf1/Screenshots/same.png",&w,&h));assert(w==16&&h==12);
    assert(!png_dimensions("/fixtures/tf1/Screenshots/bad.png",&w,&h));
    assert(!png_dimensions("/fixtures/huge.png",&w,&h));
    FILE *f=fopen(g.shots[changed].path,"ab");assert(f);fputs("changed",f);fclose(f);
    assert(gallery_delete(&g,changed)<0 && errno==ESTALE);
    assert(gallery_delete(&g,del)==0);assert(g.count==4);
    f=fopen("/fixtures/tf2/Screenshots/same.png","rb");assert(f);fclose(f);
    assert(gallery_delete(&g,999)<0 && errno==EINVAL);
    gallery_free(&g);
    assert(gallery_scan(&g,"/fixtures/missing1","/fixtures/missing2",NULL));assert(!g.count);gallery_free(&g);
    printf("PASS: %d USB transitions; UTF-8; navigation; 2-card scan, dedup, PNG limits, changed-file refusal, exact-file delete, empty cards\n",cases);
    return 0;
}
