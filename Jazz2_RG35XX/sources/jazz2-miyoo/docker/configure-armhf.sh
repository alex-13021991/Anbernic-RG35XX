#!/bin/sh
# Cross-configure for the Miyoo: Cortex-A7, software rasterizer, 320x240 logical, RGB565.
set -e
cd /work
cmake -S jazz2-native -B build-armhf -G Ninja \
    -D CMAKE_TOOLCHAIN_FILE=/work/docker/armhf-a7.cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D NCINE_PREFERRED_RHI=Software \
    -D NCINE_PREFERRED_BACKEND=SDL2 \
    -D NCINE_RHI_USE_FB16=ON \
    -D WITH_ONLINE_MULTIPLAYER=OFF \
    -D CMAKE_CXX_FLAGS="-DJAZZ2_LOGICAL_WIDTH=320 -DJAZZ2_LOGICAL_HEIGHT=240"
