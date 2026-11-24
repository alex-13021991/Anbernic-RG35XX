#include "gms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
    if (argc!=3) return 2;
    Package p={0}; Hash ver; Settings s;
    if (!strcmp(argv[2],"install") && !load_package(&p)) goto err;
    if (!enter_system(argv[1]) || !recover()) goto err;
    State state=inspect(&s,ver);
    int ok=0;
    if (!strcmp(argv[2],"state")) { printf("%d\n",state); ok=1; }
    if (!strcmp(argv[2],"install")) ok=install(&p);
    if (!strcmp(argv[2],"restore")) ok=restore();
    if (!strcmp(argv[2],"save") || !strcmp(argv[2],"stale")) {
        s.align=RIGHT; s.margin=26; s.width=290; s.scroll=0;
        if (!strcmp(argv[2],"stale")) ver[0]='x';
        ok=save(&s,ver);
    }
    free(p.main.data); free(p.additions.data);
    if (ok) return 0;
err:
    fprintf(stderr,"%s\n",problem); return 1;
}

