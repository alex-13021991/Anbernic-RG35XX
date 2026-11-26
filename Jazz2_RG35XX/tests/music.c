// SPDX-License-Identifier: GPL-3.0-or-later
#include <libopenmpt/libopenmpt.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    FILE* f = fopen(argv[1], "rb");
    if (!f || fseek(f, 0, SEEK_END) != 0) return 2;
    long size = ftell(f);
    if (size <= 0 || size > 32*1024*1024 || fseek(f, 0, SEEK_SET) != 0) return 2;
    void* bytes = malloc(size);
    if (!bytes || fread(bytes, 1, size, f) != (size_t)size) return 2;
    fclose(f);
    openmpt_module* mod = openmpt_module_create_from_memory(bytes,size,NULL,NULL,NULL);
    free(bytes);
    if (!mod) return 3;
    int16_t pcm[4096*2];
    size_t frames = 0;
    int peak = 0;
    for (unsigned i=0; i<32 && peak==0; ++i) {
        frames = openmpt_module_read_interleaved_stereo(mod,44100,4096,pcm);
        for (size_t j=0;j<frames*2;++j) {
            int value = abs((int)pcm[j]);
            if (value > peak) peak = value;
        }
    }
    openmpt_module_destroy(mod);
    if (!frames || !peak) return 4;
    printf("J2B frames=%zu peak=%d\n",frames,peak);
    ALCdevice* device = alcOpenDevice(NULL);
    ALCcontext* context = device ? alcCreateContext(device,NULL) : NULL;
    if (!context || !alcMakeContextCurrent(context)) return 5;
    ALuint buffer,source;
    alGenBuffers(1,&buffer);
    alBufferData(buffer,AL_FORMAT_STEREO16,pcm,frames*4,44100);
    alGenSources(1,&source);
    alSourcei(source,AL_BUFFER,(ALint)buffer);
    alSourcePlay(source);
    ALint state=0;
    alGetSourcei(source,AL_SOURCE_STATE,&state);
    int ok = alGetError() == AL_NO_ERROR && state == AL_PLAYING;
    printf("OpenAL %s state=%d\n",(const char*)alGetString(AL_RENDERER),state);
    alDeleteSources(1,&source);
    alDeleteBuffers(1,&buffer);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
    return ok ? 0 : 6;
}
