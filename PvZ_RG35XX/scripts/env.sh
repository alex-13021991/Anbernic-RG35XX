#!/usr/bin/env bash
PORT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
WORK="${PVZ_WORK:-$PORT/.work}"
export RG_TOOLCHAIN="${RG_TOOLCHAIN:-$HOME/RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0}"
export RG_SYSROOT="${RG_SYSROOT:-$WORK/sysroot}"
export PVZ_PORT="$PORT" LC_ALL=C
JOBS=${JOBS:-4}
CPU="-marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp"
