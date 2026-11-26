#!/bin/sh
# Resolution sweep for the software rasterizer, using the JAZZ2_RESOLUTION override.
#   sh bench-res.sh [seconds]
set -e
cd /work/build-linux-sw
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy
DURATION="${1:-25}"

for RES in 720x405 640x480 480x360 320x240; do
    printf "%-9s" "$RES"
    for LEVEL in Cache/Episodes/unknown/bench_carrot1.j2l Cache/Episodes/unknown/bench_castle1.j2l; do
        MS=$(JAZZ2_RESOLUTION="$RES" timeout "$DURATION" ./jazz2 "$LEVEL" 2>&1 \
            | sed -n 's/.*\[BENCH\] [0-9]* frames.* \([0-9]*\) us per frame/\1/p' \
            | tail -n +2 | awk '{u+=$1; n++} END {if (n) printf "%.2f", (u/n)/1000; else printf "-"}')
        printf "  %-8s ms" "$MS"
    done
    printf "\n"
done
