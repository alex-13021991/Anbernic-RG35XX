#include <fcntl.h>
#include <string.h>
int fixture_mkstemp(char *name) {
    strcpy(name+strlen(name)-6,"000001");
    return open(name,O_RDWR|O_CREAT|O_EXCL,0600);
}

