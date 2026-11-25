#include "editor.h"
#include <png.h>
#include <stdlib.h>

void free_image(Image *im) {
    free(im->p);
    *im=(Image){0};
}
int read_png(FILE *f,Image *out) {
    png_image png={0};
    png.version=PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_stdio(&png,f)) {
        png_image_free(&png);
        return fail("Не удалось прочитать заголовок PNG.");
    }
    if (png.width>8192 || png.height>8192 || (uint64_t)png.width*png.height>8388608) {
        png_image_free(&png);
        return fail("PNG слишком велик: максимум 8 Мп и 8192 px по стороне.");
    }
    png.format=PNG_FORMAT_RGBA;
    Image tmp={(int)png.width,(int)png.height,malloc(PNG_IMAGE_SIZE(png))};
    int ok=tmp.p && png_image_finish_read(&png,NULL,tmp.p,0,NULL);
    png_image_free(&png);
    if (!ok) {
        free_image(&tmp);
        return fail("Ошибка декодирования PNG или нехватка памяти.");
    }
    *out=tmp;
    return 1;
}
int write_png(FILE *f,const Image *im) {
    png_image png={0};
    png.version=PNG_IMAGE_VERSION; png.format=PNG_FORMAT_RGBA;
    png.width=im->w; png.height=im->h;
    int ok=png_image_write_to_stdio(&png,f,0,im->p,0,NULL);
    png_image_free(&png);
    return ok || fail("Не удалось записать PNG. Проверьте место на карте.");
}
