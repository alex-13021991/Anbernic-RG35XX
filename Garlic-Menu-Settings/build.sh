#!/bin/sh
set -eu
cd "$(dirname "$0")"
sdk=$(realpath "${1:-$HOME/rg35xx-sdk/miyoo-gcc-7.3.0}")
cc="$sdk/bin/arm-miyoo-linux-uclibcgnueabi-gcc"
export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
test "$("$cc" -dumpfullversion)" = "7.3.0" || { echo "Miyoo GCC 7.3.0 required" >&2; exit 1; }
root=$("$cc" -print-sysroot)
mkdir -p build
set -- -std=gnu99 -Os -Wall -Wextra -Werror -D_GNU_SOURCE -D_REENTRANT \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -marm -mcpu=cortex-a9 \
  -mfpu=neon-fp16 -mfloat-abi=softfp -Isrc -Ivendor/yyjson -Ivendor/sha256 \
  -I"$root/usr/include/SDL" -DYYJSON_DISABLE_NON_STANDARD=1 \
  src/config.c src/files.c src/transaction.c vendor/sha256/sha256.c vendor/yyjson/yyjson.c
"$cc" "$@" src/main.c src/preview.c -o build/garlic-menu-settings -s -lSDL_image -lSDL_ttf -lSDL -lpthread -lm
if [ "${GMS_TEST_BUILD:-0}" = 1 ]; then
  "$cc" "$@" -DGMS_TEST tests/cli.c -o build/core-test-arm -lm
  "$cc" "$@" -Itests -DGMS_UI_TEST src/main.c src/preview.c -o build/ui-test-arm -lSDL_image -lSDL_ttf -lSDL -lpthread -lm
  "$cc" -shared -fPIC -I"$root/usr/include/SDL" tests/pad.c -o build/test-pad.so
fi

