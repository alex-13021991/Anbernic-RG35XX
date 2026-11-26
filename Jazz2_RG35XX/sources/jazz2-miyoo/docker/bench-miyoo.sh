#!/bin/sh
# The actual Miyoo target configuration: 640x480 panel, 320x240 logical resolution,
# stretched 2x by the direct tier's presentation. Uses the build-miyoo-res binary.
set -e
cp -f /work/build-miyoo-res/jazz2 /work/build-linux-sw/jazz2-320
cd /work/build-linux-sw
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
DURATION="${1:-25}"
printf "%-10s %12s %12s\n" "level" "720x405" "640x480@320x240"
for LEVEL in Cache/Episodes/unknown/bench_*.j2l; do
    NAME=$(basename "$LEVEL" .j2l | sed 's/^bench_//')
    A=$(JAZZ2_RESOLUTION=720x405 timeout "$DURATION" ./jazz2 "$LEVEL" 2>&1 \
        | sed -n 's/.*\[BENCH\] [0-9]* frames.* \([0-9]*\) us per frame/\1/p' | tail -n +2 \
        | awk '{u+=$1;n++} END {if(n)printf "%.2f",(u/n)/1000; else printf "-"}')
    B=$(JAZZ2_RESOLUTION=640x480 timeout "$DURATION" ./jazz2-320 "$LEVEL" 2>&1 \
        | sed -n 's/.*\[BENCH\] [0-9]* frames.* \([0-9]*\) us per frame/\1/p' | tail -n +2 \
        | awk '{u+=$1;n++} END {if(n)printf "%.2f",(u/n)/1000; else printf "-"}')
    printf "%-10s %9s ms %12s ms\n" "$NAME" "$A" "$B"
done
