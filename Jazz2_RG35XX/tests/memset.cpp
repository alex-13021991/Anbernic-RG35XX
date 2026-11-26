#include <cstdio>
#include <cstring>
#include <initializer_list>
#ifdef TEST_FIXED_MEMSET
#include "../platform/RgMemset.h"
#endif
int main() {
    void* (*volatile fill)(void*, int, size_t) = &memset;
    unsigned char data[280];
    for (int value : {-128, -2, -1, 0, 128, 255, 256, 0x1234})
        for (unsigned offset=0; offset<8; ++offset)
            for (unsigned size=0; size<260; ++size) {
                for (auto& byte : data) byte=0x55;
                if (fill(data+offset,value,size) != data+offset) return 2;
                for (unsigned i=0; i<sizeof(data); ++i)
                    if (data[i] != (i>=offset && i<offset+size ? static_cast<unsigned char>(value) : 0x55)) {
                        printf("FAIL value=%d offset=%u size=%u index=%u byte=%02x\n",value,offset,size,i,data[i]);
                        return 1;
                    }
            }
    puts("memset: all values, alignments, sizes and guards OK");
}
