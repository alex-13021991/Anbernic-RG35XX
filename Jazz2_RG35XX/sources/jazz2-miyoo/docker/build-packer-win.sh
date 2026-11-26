#!/bin/sh
# Cross-builds the Windows AssetPacker: the tool players run on their own PC to turn their copy of
# Jazz Jackrabbit 2 into the content tree this port loads. Only the AssetPacker target is built - the
# game itself is never compiled for Windows here, the backend just has to be findable so that the
# top-level configure gets past its "no backend" check.
#
# The container has no network, so the prebuilt dependency set cannot be downloaded during configure.
# Stage it once from a build that did download it:
#     cp -r build-armhf/_deps/ncine_libraries-src build-deps/jazz2-libraries
# (upstream source: https://github.com/deathkiller/jazz2-libraries/archive/3.3.1.tar.gz)
set -e
cd /work

LIBS=/work/build-deps/jazz2-libraries
if [ ! -d "$LIBS/Windows" ]; then
    echo "Prebuilt Windows dependencies missing at $LIBS - see the comment at the top of this script." >&2
    exit 1
fi

cmake -S jazz2-native -B build-packer-win -G Ninja \
    -D CMAKE_TOOLCHAIN_FILE=/work/docker/mingw-w64.cmake \
    -D CMAKE_BUILD_TYPE=Release \
    -D NCINE_DOWNLOAD_DEPENDENCIES=OFF \
    -D NCINE_LIBS="$LIBS" \
    -D NCINE_PREFERRED_RHI=Software \
    -D NCINE_PREFERRED_BACKEND=SDL2 \
    -D NCINE_BUILD_ASSET_PACKER=ON \
    -D NCINE_BUILD_ASSET_PACKER_CROSS=ON \
    -D WITH_ONLINE_MULTIPLAYER=OFF \
    -D NCINE_WITH_AUDIO=OFF

cmake --build build-packer-win --target AssetPacker
