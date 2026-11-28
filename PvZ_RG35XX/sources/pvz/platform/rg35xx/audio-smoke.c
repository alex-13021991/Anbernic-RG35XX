#include <libopenmpt/libopenmpt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static int warnings=0;
static void logmessage(const char* msg, void* user) {
    (void)user;
    fprintf(stderr,"%s\n",msg);
    if (strstr(msg,"unsupported codec")) ++warnings;
}
int main(void) {
    FILE* f=fopen("test-music.mo3","rb");
    if (!f) return 2;
    fseek(f,0,SEEK_END); long len=ftell(f); rewind(f);
    void* data=malloc(len);
    if (!data || fread(data,1,len,f)!=(size_t)len) return 3;
    fclose(f);
    openmpt_module* mod=openmpt_module_create_from_memory2(data,len,logmessage,NULL,NULL,NULL,NULL,NULL,NULL);
    free(data);
    if (!mod) return 4;
    openmpt_module_set_position_seconds(mod,20.0);
    int16_t frames[4096];
    long long energy=0;
    for (int i=0;i<20;++i) {
        size_t n=openmpt_module_read_interleaved_stereo(mod,22050,2048,frames);
        for (size_t j=0;j<n*2;++j) energy+=abs(frames[j]);
    }
    printf("MUSIC samples_energy=%lld unsupported_codec_warnings=%d\n",energy,warnings);
    openmpt_module_destroy(mod);
    return energy>0 && warnings==0 ? 0 : 5;
}
