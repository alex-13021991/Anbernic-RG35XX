#!/bin/sh
# How well does the tile renderer scale across cores? The Miyoo has exactly 2.
set -e
cd /work/build-linux-sw
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy JAZZ2_RESOLUTION=640x480
for LEVEL in Cache/Episodes/unknown/bench_carrot1.j2l Cache/Episodes/unknown/bench_castle1.j2l; do
    NAME=$(basename "$LEVEL" .j2l | sed 's/^bench_//')
    MS=$(timeout "${1:-25}" ./jazz2 "$LEVEL" 2>&1 \
        | sed -n 's/.*\[BENCH\] [0-9]* frames.* \([0-9]*\) us per frame/\1/p' \
        | tail -n +2 | awk '{u+=$1; n++} END {if (n) printf "%.2f", (u/n)/1000; else printf "-"}')
    printf "%-10s %8s ms\n" "$NAME" "$MS"
done
