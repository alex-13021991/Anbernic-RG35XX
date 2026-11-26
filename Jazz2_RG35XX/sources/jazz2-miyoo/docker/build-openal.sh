#!/bin/sh
# Cross-builds OpenAL Soft for the Miyoo, with ONLY the SDL2 backend.
#
# jazz2-native can only do audio through OpenAL (ncine_options.cmake:306) and the device has no
# OpenAL and no libasound - only a bare /dev/snd. Rather than also cross-compiling ALSA and hoping
# it talks to SigmaStar's MI_AO, the SDL2 backend routes OpenAL through the device's own SDL2,
# whose "mmiyoo" audio backend drives MI_AO and is already proven by the Captain Claw port.
#
# 1.21.1 is deliberate: it needs only C++14, which the Miyoo's GCC 8.3 handles comfortably. Newer
# releases lean on C++17 library features that toolchain does not have.
set -e

VER=1.21.1
SYSROOT=/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot
OUT=/work/device-libs/openal
SDL2CPP=alc/backends/sdl2.cpp

cd /tmp
rm -rf "openal-soft-$VER"
wget -q "https://github.com/kcat/openal-soft/archive/refs/tags/$VER.tar.gz" -O openal.tar.gz
tar xf openal.tar.gz
cd "openal-soft-$VER"

# ---------------------------------------------------------------------------------------------
# Both patches below exist because the device's SDL2 audio backend makes two assumptions that
# OpenAL does not meet by default, and neither can be fixed from alsoft.conf: the SDL2 backend's
# open() runs before the configuration is applied, and its reset() then overwrites the device
# parameters with whatever SDL returned.
# ---------------------------------------------------------------------------------------------

# PATCH 1 - force a power-of-two period size.
#
# SDL hands spec.samples straight to MI_AO as u32PtNumPerFrm (SDL_audio_mmiyoo.c:110), and MI_AO
# only clocks a power-of-two frame correctly. OpenAL's default is 44100/50 = 882: SetPubAttr
# accepts it, but playback comes out torn. The device's own SDL2 is tuned for 1024 (23.2 ms),
# which is what 882 rounds up to here.
sed -i 's|want\.samples = static_cast<Uint16>(mDevice->UpdateSize);|Uint32 miyooPeriod = 64;\n    while(miyooPeriod < mDevice->UpdateSize) miyooPeriod <<= 1;\n    want.samples = static_cast<Uint16>(miyooPeriod);|' "$SDL2CPP"

# PATCH 2 - force 16-bit signed output and stop SDL renegotiating the format.
#
# This one is the important one. The device's SDL2 sizes its mix buffer as
# spec.samples * 2 * channels - two bytes per sample, hardcoded - and always programs MI_AO for
# 16 bits (SDL_audio_mmiyoo.c:98,105), but never forces spec.format to agree. OpenAL asks for
# 32-bit float by default and passes SDL_AUDIO_ALLOW_ANY_CHANGE, so SDL keeps float. SDL then
# writes twice as many bytes as that buffer holds - heap corruption, which surfaced as
# "free(): invalid pointer" a scene later - while MI_AO reads float bytes as 16-bit PCM, which is
# what the "distorted, peaking" sound actually was. One cause, both symptoms.
sed -i 's|want\.channels = (mDevice->FmtChans == DevFmtMono) ? 1 : 2;|want.channels = (mDevice->FmtChans == DevFmtMono) ? 1 : 2;\n    want.format = AUDIO_S16SYS;|' "$SDL2CPP"
sed -i 's#SDL_AUDIO_ALLOW_ANY_CHANGE#SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_CHANNELS_CHANGE#g' "$SDL2CPP"

grep -q "miyooPeriod" "$SDL2CPP" || { echo "PATCH 1 SELHAL"; exit 1; }
grep -q "want.format = AUDIO_S16SYS;" "$SDL2CPP" || { echo "PATCH 2a SELHAL"; exit 1; }
grep -q "SDL_AUDIO_ALLOW_FREQUENCY_CHANGE" "$SDL2CPP" || { echo "PATCH 2b SELHAL"; exit 1; }
echo "=== patche aplikovany ==="

rm -rf build && mkdir build && cd build
cmake .. -G Ninja \
    -D CMAKE_TOOLCHAIN_FILE=/work/docker/miyoo-device.cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX="$OUT" \
    -D ALSOFT_REQUIRE_SDL2=ON -D ALSOFT_BACKEND_SDL2=ON \
    -D ALSOFT_BACKEND_ALSA=OFF -D ALSOFT_BACKEND_OSS=OFF \
    -D ALSOFT_BACKEND_SOLARIS=OFF -D ALSOFT_BACKEND_SNDIO=OFF \
    -D ALSOFT_BACKEND_PORTAUDIO=OFF -D ALSOFT_BACKEND_PULSEAUDIO=OFF \
    -D ALSOFT_BACKEND_JACK=OFF -D ALSOFT_BACKEND_WAVE=OFF \
    -D ALSOFT_EXAMPLES=OFF -D ALSOFT_UTILS=OFF -D ALSOFT_TESTS=OFF \
    -D ALSOFT_INSTALL_EXAMPLES=OFF -D ALSOFT_INSTALL_UTILS=OFF \
    -D SDL2_INCLUDE_DIR="$SYSROOT/usr/include/SDL2" \
    -D SDL2_LIBRARY="$SYSROOT/usr/lib/libSDL2.so"
ninja && ninja install
echo "=== HOTOVO ==="
ls -la "$OUT/lib/libopenal.so.1.21.1"
