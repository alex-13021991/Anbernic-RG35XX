#!/bin/sh
set -e
cd /work
cmake --build build-linux-sw --parallel "$(nproc)"
echo "=== BUILD OK ==="
ls -la build-linux-sw/jazz2 2>/dev/null || ls -la build-linux-sw
