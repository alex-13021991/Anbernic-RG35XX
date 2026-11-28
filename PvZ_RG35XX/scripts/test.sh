#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
mkdir -p "$WORK/tests-run" "$WORK/logs"
python3 "$PORT/scripts/verify-patches.py"
python3 "$PORT/scripts/test-launcher.py"
python3 "$PORT/scripts/test-lowres.py"
g++ -std=c++17 -I "$PORT/sources/pvz/src" "$PORT/sources/pvz/tests/test_game_speed.cpp" -o "$WORK/tests-run/test-game-speed"
"$WORK/tests-run/test-game-speed"
cd "$WORK/tests-run"
for test in test_input_mapping test_viewport_scale test_setup_conversion test-lowres; do
  echo "=== $test ==="
  qemu-arm -L "$RG_SYSROOT" "$WORK/build/$test"
done
