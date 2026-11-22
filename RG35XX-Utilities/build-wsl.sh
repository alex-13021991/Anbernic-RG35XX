#!/bin/sh
set -eu
sdk=$(realpath "${1:?Usage: sh build-wsl.sh /path/to/miyoo}")
cd "$(dirname "$0")"
export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cc="$sdk/bin/arm-miyoo-linux-uclibcgnueabi-gcc"
sysroot=$("$cc" -print-sysroot)
mkdir -p build
build_app() {
    output=$1
    shift
    "$cc" -std=gnu99 -Os -flto -ffunction-sections -fdata-sections \
        -marm -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp \
        -D_GNU_SOURCE -D_REENTRANT -I"$sysroot/usr/include/SDL" \
        -Wl,--gc-sections,--as-needed -s -o "build/$output" "$@"
}
build_app screenshot-viewer src/ui.c src/gallery.c src/screenshot_viewer.c -lSDL_image -lSDL_ttf -lSDL -lpthread -lm
build_app usb-manager src/ui.c src/usb_manager.c -lSDL_ttf -lSDL -lpthread -lm
