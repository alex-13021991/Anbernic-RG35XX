#!/bin/sh
# Device build for the Miyoo Mini.
#
# Audio goes through OpenAL Soft built by docker/build-openal.sh with ONLY its SDL2 backend, so it
# rides the device's own SDL2 (mmiyoo -> MI_AO) instead of needing ALSA, which the device has no
# userspace library for. jazz2-native has no other audio path (ncine_options.cmake:306).
#
# Music needs libopenmpt: JJ2's tracks are .j2b modules, and without it the engine only logs
# "Unknown extension: j2b". Built by docker/build-openmpt.sh (0.5.x - 0.6+ requires C++17, which
# GCC 8.3 cannot give).
#
# Vorbis stays off: the packed content carries no Ogg at all and the sysroot has no vorbis headers.
set -e
cd /work

OPENAL=/work/device-libs/openal
OPENMPT=/work/device-libs/openmpt

cmake -S jazz2-native -B build-miyoo-device -G Ninja \
    -D CMAKE_TOOLCHAIN_FILE=/work/docker/miyoo-device.cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D NCINE_PREFERRED_RHI=Software \
    -D NCINE_PREFERRED_BACKEND=SDL2 \
    -D NCINE_RHI_USE_FB16=ON \
    -D NCINE_WITH_AUDIO=ON \
    -D NCINE_WITH_VORBIS=OFF \
    -D NCINE_WITH_OPENMPT=ON \
    -D OPENAL_INCLUDE_DIR="$OPENAL/include/AL" \
    -D OPENAL_LIBRARY="$OPENAL/lib/libopenal.so" \
    -D LIBOPENMPT_INCLUDE_DIR="$OPENMPT/include/libopenmpt" \
    -D LIBOPENMPT_LIBRARY="$OPENMPT/lib/libopenmpt.so" \
    -D WITH_ONLINE_MULTIPLAYER=OFF
