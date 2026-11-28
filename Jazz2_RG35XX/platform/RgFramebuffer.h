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
#if defined(__ARM_NEON)
    // Cached byte lookup indices preserve floor(x*384/640) exactly.
    uint16_t base384[80]{};
    uint8_t indices384[80][16]{};
    template<bool Wide, bool Native>
    void scale384(const uint16_t* row, void* output) const {
        const unsigned ri=Native?0:var.red.offset/8, gi=Native?1:var.green.offset/8, bi=Native?2:var.blue.offset/8;
        for(unsigned block=0;block<80;++block) {
            const uint8_t* src=reinterpret_cast<const uint8_t*>(row+base384[block]);
            uint8x8x2_t table; table.val[0]=vld1_u8(src);table.val[1]=vld1_u8(src+8);
            const uint8x8_t lo=vtbl2_u8(table,vld1_u8(indices384[block]));
            const uint8x8_t hi=vtbl2_u8(table,vld1_u8(indices384[block]+8));
            const uint16x8_t pixels=vreinterpretq_u16_u8(vcombine_u8(lo,hi));
            if(!Wide) {
                vst1q_u16(static_cast<uint16_t*>(output)+8*block,pixels);
            } else {
                const uint16x8_t r=vshrq_n_u16(pixels,11);
                const uint16x8_t g=vandq_u16(vshrq_n_u16(pixels,5),vdupq_n_u16(63));
                const uint16x8_t b=vandq_u16(pixels,vdupq_n_u16(31));
                uint8x8x4_t packed{};
                for(unsigned c=0;c<4;++c) packed.val[c]=vdup_n_u8(0);
                if(!Native && var.transp.length) packed.val[var.transp.offset/8]=vdup_n_u8(255);
                packed.val[ri]=vmovn_u16(vorrq_u16(vshlq_n_u16(r,3),vshrq_n_u16(r,2)));
                packed.val[gi]=vmovn_u16(vorrq_u16(vshlq_n_u16(g,2),vshrq_n_u16(g,4)));
                packed.val[bi]=vmovn_u16(vorrq_u16(vshlq_n_u16(b,3),vshrq_n_u16(b,2)));
                vst4_u8(static_cast<uint8_t*>(output)+32*block,packed);
            }
        }
    }
    template<bool Native>
    void expandDouble565(const uint16_t* row, uint32_t* output) const {
        const unsigned ri=Native?0:var.red.offset/8, gi=Native?1:var.green.offset/8, bi=Native?2:var.blue.offset/8;
        for (unsigned x=0; x<320; x+=8) {
            const uint16x8_t p=vld1q_u16(row+x);
            const uint16x8_t r=vshrq_n_u16(p,11);
            const uint16x8_t g=vandq_u16(vshrq_n_u16(p,5),vdupq_n_u16(63));
            const uint16x8_t b=vandq_u16(p,vdupq_n_u16(31));
            const uint8x8_t r8=vmovn_u16(vorrq_u16(vshlq_n_u16(r,3),vshrq_n_u16(r,2)));
            const uint8x8_t g8=vmovn_u16(vorrq_u16(vshlq_n_u16(g,2),vshrq_n_u16(g,4)));
            const uint8x8_t b8=vmovn_u16(vorrq_u16(vshlq_n_u16(b,3),vshrq_n_u16(b,2)));
            const uint8x8x2_t rr=vzip_u8(r8,r8), gg=vzip_u8(g8,g8), bb=vzip_u8(b8,b8);
            uint8x8x4_t lo{}, hi{};
            for(unsigned c=0;c<4;++c) lo.val[c]=hi.val[c]=vdup_n_u8(0);
            if(!Native && var.transp.length) lo.val[var.transp.offset/8]=hi.val[var.transp.offset/8]=vdup_n_u8(255);
            lo.val[ri]=rr.val[0]; hi.val[ri]=rr.val[1];
            lo.val[gi]=gg.val[0]; hi.val[gi]=gg.val[1];
            lo.val[bi]=bb.val[0]; hi.val[bi]=bb.val[1];
            vst4_u8(reinterpret_cast<uint8_t*>(output+2*x),lo);
            vst4_u8(reinterpret_cast<uint8_t*>(output+2*x+8),hi);
        }
    }

#endif
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
#if defined(__ARM_NEON)
            if(width==384) for(unsigned block=0;block<80;++block) {
                const unsigned first=xmap[8*block];
                base384[block]=first<376?first:376; // Eight-pixel loads never cross the source row.
                for(unsigned i=0;i<8;++i) {
                    const unsigned byte=2*(xmap[8*block+i]-base384[block]);
                    indices384[block][2*i]=byte;
                    indices384[block][2*i+1]=byte+1;
                }
            }
#endif
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
                if(width==384) {
                    if(bytes==2) scale384<false,true>(row,line);
                    else if(var.red.offset==0 && var.green.offset==8 && var.blue.offset==16 && !var.transp.length)
                        scale384<true,true>(row,line32);
                    else scale384<true,false>(row,line32);
                } else
                if (bytes == 4 && width == 320) {
                    // Fuse RGB565 expansion, channel packing and exact 2x scaling.
                    if (var.red.offset==0 && var.green.offset==8 && var.blue.offset==16 && !var.transp.length)
                        expandDouble565<true>(row,line32);
                    else expandDouble565<false>(row,line32);
                } else
#endif
                {
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
