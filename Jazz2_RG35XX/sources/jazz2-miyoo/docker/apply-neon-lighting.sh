#!/bin/sh
# Splices the NEON variant of combineLightingScanline (docker/neon-lighting-variant.cpp) into
# SwRaster.cpp, next to the Sse2T/Avx2T ones. The dispatcher resolves Cpu::DefaultBase at compile
# time, so the overload alone is enough to make ARM builds use it.
#
# Kept out of the tree because it did NOT reduce frame time in castle1 (12.1-13.1 ms against a
# 12.4-12.5 ms scalar baseline) despite the op itself measuring 2x faster - see NOTES step 13.
# It is here so the same build can be A/B'd against a level where lighting actually dominates.
#
# Undo with: git checkout Sources/nCine/Graphics/RHI/Software/SwRaster.cpp
set -e
cd "$(dirname "$0")/../jazz2-native"
F=Sources/nCine/Graphics/RHI/Software/SwRaster.cpp
grep -q "combineLightingScanlineImplementation(Cpu::NeonT)" "$F" && { echo "uz je aplikovano"; exit 0; }
L=$(grep -n "DEATH_CPU_DISPATCHER_BASE(combineLightingScanlineImplementation)" "$F" | cut -d: -f1)
E=$((L - 3))
head -n "$E" "$F" > /tmp/sw.cpp
cat ../docker/neon-lighting-variant.cpp >> /tmp/sw.cpp
tail -n +$((E + 1)) "$F" >> /tmp/sw.cpp
mv /tmp/sw.cpp "$F"
grep -c "combineLightingScanlineImplementation(Cpu::NeonT)" "$F"
echo "NEON varianta aplikovana"
