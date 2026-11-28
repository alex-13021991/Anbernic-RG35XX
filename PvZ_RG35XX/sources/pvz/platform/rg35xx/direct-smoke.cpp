#include "Platform.h"
#include <cstdint>
int main() {
    static uint16_t pixels[640*480];
    for(int y=0;y<480;y++)for(int x=0;x<640;x++) pixels[y*640+x]= y<240 ? 0xf800 : 0x001f;
    Rg35xxPresentPixels(pixels);
    return 0;
}
