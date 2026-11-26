#!/bin/sh
# Stages a level from the prebaked tree as Content/Episodes/unknown/bench_<name>.j2l, which is the
# only way a Release build can be pointed at a specific level: the /level switch sits behind
# DEATH_DEBUG (Main.cpp), but a .j2l path on the command line resolves to "unknown/<name>".
#
#   sh stage-bench-level.sh prince/03_carrot1        -> out-miyoo/.../bench_carrot1.j2l
#
# Then upload that one file to the device and run:
#   ./jazz2 "Content/Episodes/unknown/bench_carrot1.j2l"
set -e
cd "$(dirname "$0")/.."
SRC="out-miyoo/Episodes/$1.j2l"
[ -f "$SRC" ] || { echo "neni: $SRC"; echo "dostupne:"; ls out-miyoo/Episodes/*/ | head -40; exit 1; }
NAME=$(basename "$1" | sed 's/^[0-9]*_//')
DST="out-miyoo/Episodes/unknown/bench_$NAME.j2l"
mkdir -p out-miyoo/Episodes/unknown
cp -f "$SRC" "$DST"
cp -f "$SRC" "MiyooPackage/Roms/PORTS/Games/Jazz Jackrabbit 2/Content/Episodes/unknown/bench_$NAME.j2l"
echo "pripraveno: bench_$NAME.j2l"
