#!/bin/sh
# Clones jazz2-native at the exact upstream commit this port is built against and applies the
# port's patch series. The engine is not vendored here: the changes are 438 lines across 17 files,
# and keeping them as a patch makes them reviewable as changes rather than buried in a fork.
#
# Upstream is GPL-3.0, so these patches are too - that is what makes this repository, together with
# the pinned commit below, the "corresponding source" for the binary in the release.
set -e
cd "$(dirname "$0")/.."

UPSTREAM=https://github.com/deathkiller/jazz2-native.git
COMMIT=b8cba70

if [ -d jazz2-native ]; then
    echo "jazz2-native/ already exists - remove it first to re-fetch"
    exit 1
fi

git clone "$UPSTREAM" jazz2-native
cd jazz2-native
git checkout "$COMMIT"
for p in ../patches/*.patch; do
    echo "Applying $(basename "$p")"
    git apply "$p"
done
echo "Engine ready at $COMMIT with the port patches applied."
