#!/bin/sh
# Dynamic ARM instruction count per rendered frame.
#
# qemu's wall-clock timing is emulation speed on the host and says nothing about a Cortex-A7,
# but the number of guest instructions executed is real. Startup and level loading would swamp
# a single run, so each configuration is run twice at different durations and the steady-state
# cost is the DIFFERENCE: (insn_long - insn_short) / (frames_long - frames_short).
set -e
cd /work/build-linux-sw
cp -f /work/build-armhf/jazz2 ./jazz2-arm
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy

SHORT="${1:-30}"
LONG="${2:-60}"

run() { # $1=seconds $2=resolution $3=level -> "<insns> <frames>"
    JAZZ2_RESOLUTION="$2" timeout "$1" qemu-arm-plugins -d plugin -plugin /opt/libinsncount.so \
        ./jazz2-arm "Cache/Episodes/unknown/$3.j2l" 2>&1 \
        | awk '
            /INSN\] executed:/ { insn = $NF }
            /BENCH\]/ { for (i = 1; i <= NF; i++) if ($i == "frames") { frames += $(i-1); break } }
            END { print insn + 0, frames + 0 }'
}

printf "%-10s %-10s %16s %9s\n" "level" "okno" "instr/snimek" "snimku"
for RES in 320x240 640x480; do
    for LEVEL in bench_castle1 bench_carrot1; do
        A=$(run "$SHORT" "$RES" "$LEVEL")
        B=$(run "$LONG"  "$RES" "$LEVEL")
        printf "%s %s\n" "$A" "$B" | awk -v l="$(echo "$LEVEL" | sed s/bench_//)" -v r="$RES" '
            { di = $3 - $1; df = $4 - $2
              if (df > 0) printf "%-10s %-10s %16.0f %9d\n", l, r, di / df, df
              else        printf "%-10s %-10s %16s %9s\n", l, r, "?", "?" }'
    done
done
