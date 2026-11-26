#!/bin/sh
# Runs the software-rendered build headless and reports peak RSS.
# The Miyoo Mini has 128 MB total, so the number to watch is "Maximum resident set size".
#
# Needs original Jazz Jackrabbit 2 files in /work/jazz2-native/Content/Source/
# (or next to the binary in build-linux-sw/Content/Source/).
set -e
cd /work/build-linux-sw

export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

DURATION="${1:-60}"
echo "=== running ./jazz2 for ${DURATION}s (headless) ==="
/usr/bin/time -v timeout "${DURATION}" ./jazz2 2>&1 | tail -30
