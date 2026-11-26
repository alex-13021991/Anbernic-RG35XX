#!/bin/sh
# Cross-builds libopenmpt for the Miyoo. This is what plays the game's music: JJ2's tracks are
# .j2b module files, and without it the engine just logs "Unknown extension: j2b".
#
# 0.5.x is deliberate. libopenmpt 0.6 and newer require C++17 in a way GCC 8.3 (the Miyoo
# toolchain) cannot satisfy; 0.5 needs only C++11. jazz2-native's own fallback would fetch 0.7.1,
# which is why the build must be pointed at this one explicitly.
#
# Built through libopenmpt's plain Makefile rather than autotools: every optional dependency is
# switched off, so the result needs nothing but libc/libstdc++ - no zlib, mpg123, ogg/vorbis,
# sndfile, PulseAudio or SDL. The engine decodes Ogg separately and does not need libopenmpt's.
set -e

VER=0.5.34
SYSROOT=/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot
OUT=/work/device-libs/openmpt

cd /tmp
rm -rf "libopenmpt-$VER+release.autotools" openmpt.tar.gz
wget -q "https://lib.openmpt.org/files/libopenmpt/src/libopenmpt-$VER+release.autotools.tar.gz" -O openmpt.tar.gz
tar xf openmpt.tar.gz
cd "libopenmpt-$VER+release.autotools"

# The sysroot ships zlib.pc, but cross pkg-config will not look there on its own - and zlib is not
# optional here: the compressed .j2b variant JJ2 uses is inflated through it.
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"

./configure \
    --host=arm-linux-gnueabihf \
    --prefix="$OUT" \
    --with-sysroot="$SYSROOT" \
    --disable-openmpt123 --disable-examples --disable-tests --disable-doxygen-doc \
    --without-mpg123 --without-ogg --without-vorbis --without-vorbisfile \
    --without-portaudio --without-portaudiocpp --without-pulseaudio --without-sndfile \
    --without-flac --disable-static --enable-shared \
    CFLAGS="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -O2" \
    CXXFLAGS="-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -O2"

make -j"$(nproc)"
make install
echo "=== HOTOVO ==="
ls -la "$OUT/lib/" "$OUT/include/libopenmpt/" 2>/dev/null | head -20
