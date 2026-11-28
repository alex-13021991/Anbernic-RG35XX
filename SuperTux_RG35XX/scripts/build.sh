#!/usr/bin/env bash
set -euo pipefail
PORT=$(cd "$(dirname "$0")/.." && pwd)
variant=${1:-ru}
case "$variant" in en|ru) ;; *) echo 'Использование: build.sh en|ru' >&2; exit 2;; esac
WORK=${SUPERTUX_WORK:-"$PORT/.work"}
SDK=${SDK:-"$HOME/RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0"}
SYSROOT=${SYSROOT:-"$WORK/sysroot"}
ARCHIVE=${SYSROOT_ARCHIVE:-"$HOME/RG35XX/RG35XX-Development/Sysroot/20261125T155126Z-sysroot.tar.gz"}
mkdir -p "$WORK/logs"
python3 "$PORT/scripts/prepare-source.py" "$variant" --work "$WORK"
if [[ ! -f "$SYSROOT/usr/include/SDL/SDL.h" ]]; then
    mkdir -p "$SYSROOT"
    tar xzf "$ARCHIVE" -C "$SYSROOT"
fi
make -C "$WORK/$variant/source" -f Makefile.rg35xx SDK="$SDK" SYSROOT="$SYSROOT"     -j"${JOBS:-8}" 2>&1 | tee "$WORK/logs/build-$variant.txt"
printf 'Готово: %s\n' "$WORK/$variant/source/supertux"
