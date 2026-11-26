#!/bin/sh
# Configure jazz2-native for the CPU rasterizer, SDL2 window backend, no online multiplayer.
set -e
cd /work
cmake -S jazz2-native -B build-linux-sw -G Ninja \
    -D CMAKE_BUILD_TYPE=Release \
    -D NCINE_PREFERRED_RHI=Software \
    -D NCINE_PREFERRED_BACKEND=SDL2 \
    -D WITH_ONLINE_MULTIPLAYER=OFF
