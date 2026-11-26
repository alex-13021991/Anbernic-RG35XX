#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
[[ -d "$WORK/engine" ]] || bash "$PORT/scripts/source.sh"
[[ -d "$WORK/zlib-1.2.11" ]] || tar xf "$VENDOR/zlib-1.2.11.tar.gz" -C "$WORK"
mode=${1:-linux}
extra=()
case "$mode" in
    linux) ;;
    windows) extra=(-DCMAKE_TOOLCHAIN_FILE="$PORT/cmake/mingw.cmake") ;;
    *) echo 'Usage: build-converter.sh [linux|windows]' >&2; exit 1 ;;
esac
unset LD_LIBRARY_PATH PKG_CONFIG_LIBDIR PKG_CONFIG_SYSROOT_DIR
cmake -S "$PORT/cmake/converter" -B "$WORK/converter-$mode" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DENGINE_DIR="$WORK/engine" -DZLIB_SOURCE_DIR="$WORK/zlib-1.2.11" "${extra[@]}"
cmake --build "$WORK/converter-$mode" --target AssetPacker --parallel "$JOBS"
