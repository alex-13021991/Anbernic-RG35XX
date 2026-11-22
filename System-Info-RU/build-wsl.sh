#!/bin/sh
set -eu
sdk=$(realpath "${1:?Usage: sh build-wsl.sh /path/to/miyoo}")
cd "$(dirname "$0")"
export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc="$sdk/bin/arm-miyoo-linux-uclibcgnueabi-gcc"
sysroot=$("$cc" -print-sysroot)
mkdir -p build
"$cc" -std=gnu99 -Os -flto -ffunction-sections -fdata-sections \
    -marm -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp \
    -D_GNU_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64 -I"$sysroot/usr/include/SDL" \
    -Wl,--gc-sections,--as-needed -s -o build/system-info \
    src/main.c src/ui.c src/screens.c src/system.c src/storage.c \
    src/catalog.c src/cache.c -lSDL_ttf -lSDL -lpthread -lm
