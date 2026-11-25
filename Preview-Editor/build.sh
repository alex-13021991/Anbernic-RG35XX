#!/bin/sh
set -eu
cd "$(dirname "$0")"
sdk=$(realpath "${1:-$HOME/rg35xx-sdk/miyoo-gcc-7.3.0}")
cc="$sdk/bin/arm-miyoo-linux-uclibcgnueabi-gcc"
export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
root=$("$cc" -print-sysroot)
mkdir -p build
cp -L "$root/usr/lib/libpng16.so.16" build/libpng16.so.16
sed -n '1,/END OF COPYRIGHT NOTICE, DISCLAIMER, and LICENSE/p' "$root/usr/include/libpng16/png.h" > build/libpng-license.txt
set -- -std=gnu99 -Os -Wall -Wextra -Werror -D_GNU_SOURCE -D_REENTRANT \
    -marm -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -Isrc -Ivendor/sha256 -Ivendor/yyjson \
    -I"$root/usr/include/SDL" -DYYJSON_DISABLE_WRITER=1 -DYYJSON_DISABLE_NON_STANDARD=1
"$cc" "$@" src/*.c vendor/sha256/sha256.c vendor/yyjson/yyjson.c \
    -o build/preview-editor -s -lSDL_ttf -lSDL -lpng16 -lpthread -lm
if [ "${PREVIEW_TESTS:-0}" = 1 ]; then
    "$cc" "$@" tests/check.c src/editor.c src/png_io.c src/storage.c src/theme.c \
        vendor/sha256/sha256.c vendor/yyjson/yyjson.c -o build/check -lSDL_ttf -lSDL -lpng16 -lpthread -lm
    "$cc" "$@" -shared -fPIC tests/pad.c -o build/pad.so -lSDL
fi
