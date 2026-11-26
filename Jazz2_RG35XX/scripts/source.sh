#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
[[ ! -e "$WORK/engine" ]] || { echo "Already exists: $WORK/engine; choose a new JAZZ2_WORK." >&2; exit 1; }
(cd "$VENDOR" && sha256sum -c SHA256SUMS)
mkdir -p "$WORK"
staging=$(mktemp -d "$WORK/source.XXXXXX")
tar xf "$VENDOR/jazz2-native-b8cba705.tar.gz" -C "$staging"
for patch in "$PORT"/patches/{miyoo,rg35xx}/*.patch; do
    (cd "$staging" && git apply --check "$patch" && git apply "$patch")
done
cp "$PORT"/platform/*.h "$staging/Sources/nCine/Backends/"
mv "$staging" "$WORK/engine"
echo "Patched source: $WORK/engine"
