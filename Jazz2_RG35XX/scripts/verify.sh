#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
export PYTHONDONTWRITEBYTECODE=1 LC_ALL=C
mkdir -p "$WORK/tests" "$WORK/reports"
cc="$RG_TOOLCHAIN/bin/$TRIPLE-gcc"
cxx="$RG_TOOLCHAIN/bin/$TRIPLE-g++"
bin="$WORK/build/jazz2"
lib="$RG_SYSROOT/usr/lib/libopenmpt.so.0"
export LD_LIBRARY_PATH="$RG_TOOLCHAIN/lib"
flags=(--sysroot="$RG_SYSROOT" -marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp -O2)
link=(-Wl,-dynamic-linker,/lib/ld-uClibc.so.0,-rpath-link,"$RG_SYSROOT/usr/lib",-rpath-link,"$RG_SYSROOT/lib")
"$cxx" "${flags[@]}" -std=gnu++17 -I"$RG_SYSROOT/usr/include/SDL2" "$PORT/tests/video.cpp" -o "$WORK/tests/video" -lSDL2 "${link[@]}"
"$cc" "${flags[@]}" "$PORT/tests/music.c" -o "$WORK/tests/music" -lopenmpt -lopenal "${link[@]}"
"$cc" "${flags[@]}" -fPIC -shared "$PORT/tests/simulated-io.c" -ldl -o "$WORK/tests/simulated-io.so"
"$cxx" "${flags[@]}" -std=gnu++17 -DTEST_FIXED_MEMSET "$PORT/tests/memset.cpp" -o "$WORK/tests/memset-fixed" "${link[@]}"
unset LD_LIBRARY_PATH
qemu-arm -cpu cortex-a9 -L "$RG_SYSROOT" "$WORK/tests/memset-fixed"
python3 "$PORT/tests/framebuffer.py" "$WORK"
for elf in "$bin" "$WORK/tests/video" "$WORK/tests/music"; do
    python3 "$PORT/scripts/abi.py" "$elf" "$RG_SYSROOT" > "$WORK/reports/${elf##*/}.abi.txt"
done
for elf in "$bin" "$lib"; do
    readelf -WhAldVWs "$elf" > "$WORK/reports/${elf##*/}.readelf.txt"
    arm-linux-gnueabi-objdump -d "$elf" > "$WORK/reports/disassembly.tmp"
    if grep -Ei '[[:space:]](sdiv|udiv|vfma|vfms|vfnma|vfnms)([.[:space:]])' "$WORK/reports/disassembly.tmp"; then
        echo "Обнаружена инструкция вне профиля Cortex-A9" >&2
        exit 1
    fi
done
if [[ $# -gt 0 ]]; then
    content=$(realpath "$1")
    qemu-arm -cpu cortex-a9 -L "$RG_SYSROOT" -E ALSOFT_DRIVERS=null "$WORK/tests/music" "$content/Music/menu.j2b"
    python3 "$PORT/tests/smoke.py" "$WORK" "$content"
    python3 "$PORT/tests/smoke.py" "$WORK" "$content" --level prince/01_castle1 --garlic
    python3 "$PORT/tests/smoke.py" "$WORK" "$content" --bpp 16
    python3 "$PORT/tests/smoke.py" "$WORK" "$content" --padded
    python3 "$PORT/tests/smoke.py" "$WORK" "$content" --bpp 24
fi
