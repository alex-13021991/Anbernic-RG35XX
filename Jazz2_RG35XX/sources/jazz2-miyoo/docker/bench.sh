#!/bin/sh
# Frame-rate benchmark for the software rasterizer.
#
#   sh bench.sh [seconds]
#
# Release builds have no /level switch (it sits behind DEATH_DEBUG, Main.cpp:462), but they do
# accept a .j2l path, which resolves to "unknown/<name>" (Main.cpp:447). So the levels to measure
# are staged into Cache/Episodes/unknown/ as bench_*.j2l.
#
# Reports the [BENCH] lines the patched FrameTimer emits (docker/bench-fps note in NOTES.md).
#
# NOTE: this runs on the x86 host, NOT on a Cortex-A7. Absolute FPS is meaningless for the Miyoo;
# what transfers is the RELATIVE cost between levels and resolutions. Run with --cpus=2 to at
# least match the core count.
set -e
cd /work/build-linux-sw

export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

DURATION="${1:-30}"

for LEVEL in Cache/Episodes/unknown/bench_*.j2l; do
    NAME=$(basename "$LEVEL" .j2l | sed 's/^bench_//')
    # First interval is the load spike, so drop it and average the rest
    RESULT=$(timeout "$DURATION" ./jazz2 "$LEVEL" 2>&1 \
        | sed -n 's/.*\[BENCH\] \([0-9]*\) frames.* \([0-9]*\) us per frame/\1 \2/p' \
        | tail -n +2 \
        | awk '{f+=$1; u+=$2; n++} END {if (n) printf "%6.1f FPS   %6.2f ms/frame   (%d samples)", 1000000/(u/n), (u/n)/1000, n; else print "no samples"}')
    printf "%-12s %s\n" "$NAME" "$RESULT"
done
