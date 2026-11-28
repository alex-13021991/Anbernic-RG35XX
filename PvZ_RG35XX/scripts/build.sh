#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
bash "$PORT/scripts/prepare.sh"
export LD_LIBRARY_PATH="$RG_TOOLCHAIN/lib"
cmake -S "$PORT/sources/pvz" -B "$WORK/build" -G Ninja  -DCMAKE_TOOLCHAIN_FILE="$PORT/cmake/garlic.cmake" -DCMAKE_BUILD_TYPE=Release  -DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>&1 | tee "$WORK/logs/configure.log"
cmake --build "$WORK/build" --target PvZ setup_miyoo test_input_mapping test_viewport_scale test_setup_conversion --parallel "$JOBS" 2>&1 | tee "$WORK/logs/build.log"
python3 "$PORT/scripts/abi.py" "$WORK/build/PvZ" "$RG_SYSROOT" > "$WORK/logs/abi.txt"
"$RG_TOOLCHAIN/bin/arm-linux-g++" --version > "$WORK/logs/compiler.txt"
