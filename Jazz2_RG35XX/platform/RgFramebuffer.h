// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <ctime>
#include <unistd.h>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace Rg35xx {
class Framebuffer {
    int fd = -1;
    void* mapping = MAP_FAILED;
    fb_fix_screeninfo fix{};
    fb_var_screeninfo var{};
    unsigned xmap[640]{};
    int cachedWidth = 0;
    uint32_t red[32]{}, green[64]{}, blue[32]{};
    uint32_t alpha = 0;
    double reportStart = 0;
    unsigned reportFrames = 0;
public:
    Framebuffer() = default;
    Framebuffer(const Framebuffer&) = delete;
    ~Framebuffer() {
        if (mapping != MAP_FAILED) munmap(mapping, fix.smem_len);
        if (fd >= 0) close(fd);
    }
    bool openDisplay() {
        fd = open("/dev/fb0", O_RDWR | O_CLOEXEC);
        if (fd < 0 || ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0 ||
            ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
            perror("fb0");
            return false;
        }
        const uint64_t right = uint64_t(var.xoffset) + var.xres;
        const uint64_t bottom = uint64_t(var.yoffset) + var.yres;
        const bool rgb565 = var.bits_per_pixel == 16 &&
            var.red.offset == 11 && var.red.length == 5 && var.green.offset == 5 &&
            var.green.length == 6 && var.blue.offset == 0 && var.blue.length == 5 && !var.transp.length;
        const bool rgb32 = var.bits_per_pixel == 32 &&
            var.red.length == 8 && var.green.length == 8 && var.blue.length == 8 &&
            var.red.offset <= 24 && var.green.offset <= 24 && var.blue.offset <= 24 &&
            var.red.offset % 8 == 0 && var.green.offset % 8 == 0 && var.blue.offset % 8 == 0 &&
            var.red.offset != var.green.offset && var.red.offset != var.blue.offset && var.green.offset != var.blue.offset &&
            (!var.transp.length || (var.transp.length == 8 && var.transp.offset <= 24 && var.transp.offset % 8 == 0 &&
             var.transp.offset != var.red.offset && var.transp.offset != var.green.offset && var.transp.offset != var.blue.offset));
        const unsigned bytes = var.bits_per_pixel / 8;
        if (var.xres != 640 || var.yres != 480 || (!rgb565 && !rgb32) ||
            fix.type != FB_TYPE_PACKED_PIXELS || fix.visual != FB_VISUAL_TRUECOLOR ||
            var.nonstd || var.grayscale || var.red.msb_right || var.green.msb_right || var.blue.msb_right || var.transp.msb_right ||
            right > var.xres_virtual || bottom > var.yres_virtual ||
            right * bytes > fix.line_length || fix.line_length % bytes ||
            bottom * fix.line_length > fix.smem_len) {
            fprintf(stderr,"fb0: unsupported geometry %ux%u bpp=%u stride=%u\n",
                    var.xres,var.yres,var.bits_per_pixel,fix.line_length);
            return false;
        }
        mapping = mmap(nullptr,fix.smem_len,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
        if (mapping == MAP_FAILED) { perror("fb0 mmap"); return false; }
        if (rgb32) {
            for (unsigned i=0; i<32; ++i) {
                red[i] = ((i << 3) | (i >> 2)) << var.red.offset;
                blue[i] = ((i << 3) | (i >> 2)) << var.blue.offset;
            }
            for (unsigned i=0; i<64; ++i) green[i] = ((i << 2) | (i >> 4)) << var.green.offset;
            if (var.transp.length) alpha = uint32_t(255) << var.transp.offset;
        }
        fprintf(stderr,"fb0: %u-bit RGB %ux%u stride=%u offset=%u,%u channels=%u/%u/%u alpha=%u\n",
                var.bits_per_pixel,var.xres,var.yres,fix.line_length,var.xoffset,var.yoffset,
                var.red.offset,var.green.offset,var.blue.offset,var.transp.length);
        return true;
    }
    // Буфер SwRaster хранит нижнюю строку первой. Копируем только видимую страницу.
    void present(const void* pixels,int width,int height,int stride) {
        if (mapping == MAP_FAILED || !pixels || width <= 0 || height <= 0 || stride / 2 < width) return;
        if (width != cachedWidth) {
            fprintf(stderr,"RG35XX: presenting %dx%d -> 640x480\n",width,height);
            for (unsigned x=0; x<640; ++x) xmap[x] = x * unsigned(width) / 640;
            cachedWidth = width;
        }
        const unsigned bytes = var.bits_per_pixel / 8;
        auto* dst = static_cast<uint8_t*>(mapping) + var.yoffset * fix.line_length + var.xoffset * bytes;
        uint16_t line[640];
        uint32_t line32[640];
        int lastY = -1;
        for (unsigned y=0; y<480; ++y, dst+=fix.line_length) {
            int srcY = height - 1 - int(y * unsigned(height) / 480);
            if (srcY != lastY) {
                const auto* row = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(pixels) + srcY * stride);
#if defined(__ARM_NEON)
                if (width == 320) {
                    // Exact 2x scaling: duplicate eight RGB565 pixels per load.
                    for (unsigned x=0; x<320; x+=8) {
                        const uint16x8_t p = vld1q_u16(row+x);
                        const uint16x8x2_t doubled = vzipq_u16(p,p);
                        vst1q_u16(line+2*x,doubled.val[0]);
                        vst1q_u16(line+2*x+8,doubled.val[1]);
                    }
                } else
#endif
                {
                    for (unsigned x=0; x<640; ++x) line[x] = row[xmap[x]];
                }
                if (bytes == 4) for (unsigned x=0; x<640; ++x) {
                    const unsigned p = line[x];
                    line32[x] = red[p >> 11] | green[(p >> 5) & 63] | blue[p & 31] | alpha;
                }
                lastY = srcY;
            }
            memcpy(dst,bytes == 2 ? static_cast<void*>(line) : static_cast<void*>(line32),640 * bytes);
        }
        timespec now{};
        clock_gettime(CLOCK_MONOTONIC, &now);
        const double seconds = now.tv_sec + now.tv_nsec * 1e-9;
        if (!reportStart) reportStart = seconds;
        ++reportFrames;
        if (seconds - reportStart >= 5) {
            rusage usage{};
            getrusage(RUSAGE_SELF, &usage);
            fprintf(stderr,"RG35XX: present_fps=%.2f maxrss_kib=%ld\n",
                    reportFrames / (seconds - reportStart), usage.ru_maxrss);
            reportFrames = 0; reportStart = seconds;
        }
    }
    void probeTiming() {
        // Запрос текущего смещения: дисплей не переключается на другую страницу.
        fb_var_screeninfo same = var;
        errno = 0;
        int rc = ioctl(fd, FBIOPAN_DISPLAY, &same);
        fprintf(stderr,"FBIOPAN_DISPLAY (current page): %d errno=%d\n",rc,errno);
    }
};
}
