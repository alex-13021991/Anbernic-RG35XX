#include "editor.h"
#include <stdlib.h>
#include <string.h>

int clamp(int value,int low,int high) {
    return value<low ? low : value>high ? high : value;
}
int output_size(int pixels,int percent) {
    int size=(pixels*percent+50)/100;
    return size>0 ? size : 1;
}
/* First destination pixel sampling a given source coordinate, nearest neighbour. */
static int boundary(int source,int dest_size,int source_size) {
    return (source*dest_size+source_size-1)/source_size;
}
int visible_bounds(const Image *img,Placement p,Rect *bounds) {
    Rect c=p.crop;
    int w=output_size(c.w,p.percent),h=output_size(c.h,p.percent);
    int left=w,right=0,top=h,bottom=0;
    /* x[i] is the first output column sampling source column i. Source
       sides are limited to 8192 and the maximum zoom is 400 percent. */
    int xedge[8193];
    for (int x=0;x<=c.w;x++) xedge[x]=boundary(x,w,c.w);
    for (int y=0;y<c.h;y++) {
        int y0=boundary(y,h,c.h), y1=boundary(y+1,h,c.h);
        if (y1==y0) continue;
        const Pixel *line=img->p+(c.y+y)*img->w+c.x;
        int a=0,b=c.w-1;
        while (a<=b && (!line[a].a || xedge[a]==xedge[a+1])) a++;
        if (a>b) continue;
        while (b>a && (!line[b].a || xedge[b]==xedge[b+1])) b--;
        if (left>xedge[a]) left=xedge[a];
        if (right<xedge[b+1]) right=xedge[b+1];
        if (top>y0) top=y0;
        if (bottom<y1) bottom=y1;
    }
    if (right<=left) return 0;
    *bounds=(Rect){left,top,right-left,bottom-top};
    return 1;
}
int initial(const Image *img,Placement *out) {
    Placement p={{0,0,img->w,img->h},0,0,100};
    if (!visible_bounds(img,p,&p.crop)) return 0;
    if (img->w==CW && img->h==CH) {
        p.x=p.crop.x; p.y=p.crop.y;
    } else {
        int sx=CW*100/p.crop.w,sy=CH*100/p.crop.h;
        p.percent=clamp(sx<sy?sx:sy,1,100);
        p.x=(CW-output_size(p.crop.w,p.percent))/2;
        p.y=(CH-output_size(p.crop.h,p.percent))/2;
    }
    *out=p;
    return 1;
}
void shift(Placement *p,int dx,int dy) {
    p->x=clamp(p->x+dx,-65536,65536);
    p->y=clamp(p->y+dy,-65536,65536);
}
void scale(Placement *p,int percent) {
    int next=clamp(percent,1,400);
    int oldw=output_size(p->crop.w,p->percent),oldh=output_size(p->crop.h,p->percent);
    shift(p,oldw/2-output_size(p->crop.w,next)/2,oldh/2-output_size(p->crop.h,next)/2);
    p->percent=next;
}
void trim(Placement *p,const Image *img,int edge,int amount) {
    Rect *c=&p->crop;
    int horizontal=edge==0 || edge==2;
    int *origin=horizontal ? &c->x : &c->y, *size=horizontal ? &c->w : &c->h;
    int limit=horizontal ? img->w : img->h;
    int leading=edge<2;
    int change=clamp(amount,leading ? -*origin : *origin+*size-limit,*size-1);
    int old=output_size(*size,p->percent);
    *size-=change;
    if (leading) {
        *origin+=change;
        int delta=old-output_size(*size,p->percent);
        shift(p,horizontal ? delta : 0,horizontal ? 0 : delta);
    }
}
int align_image(const Image *img,Placement *p,int cell,int margin,int top,int bottom) {
    Rect b;
    if (!visible_bounds(img,*p,&b)) return 0;
    int col=cell%3,row=cell/3;
    int x=col==0 ? margin : col==1 ? (CW-b.w)/2 : CW-margin-b.w;
    int y=row==0 ? top+margin : row==1 ? top+(CH-top-bottom-b.h)/2 : CH-bottom-margin-b.h;
    p->x=x-b.x; p->y=y-b.y;
    return 1;
}
void render(const Image *img,Placement p,Pixel *canvas) {
    Rect c=p.crop;
    int w=output_size(c.w,p.percent),h=output_size(c.h,p.percent);
    int left=clamp(p.x,0,CW),right=clamp(p.x+w,0,CW);
    int top=clamp(p.y,0,CH),bottom=clamp(p.y+h,0,CH);
    memset(canvas,0,sizeof(Pixel)*CW*CH);
    int columns[CW];
    for (int x=left;x<right;x++) columns[x]=c.x+(int)((int64_t)(x-p.x)*c.w/w);
    for (int y=top;y<bottom;y++) {
        int row=c.y+(int)((int64_t)(y-p.y)*c.h/h);
        const Pixel *src=img->p+row*img->w;
        for (int x=left;x<right;x++) canvas[y*CW+x]=src[columns[x]];
    }
}
int same_place(Placement a,Placement b) {
    return a.x==b.x && a.y==b.y && a.percent==b.percent && a.crop.x==b.crop.x &&
        a.crop.y==b.crop.y && a.crop.w==b.crop.w && a.crop.h==b.crop.h;
}
void history_push(History *h,Placement next) {
    if (same_place(h->list[h->at],next)) return;
    if (h->at==HISTORY-1) {
        memmove(h->list,h->list+1,(HISTORY-1)*sizeof next);
        h->at--;
    }
    h->list[++h->at]=next;
    h->last=h->at;
}
int history_move(History *h,int step) {
    int next=clamp(h->at+step,0,h->last);
    if (next==h->at) return 0;
    h->at=next;
    return 1;
}
