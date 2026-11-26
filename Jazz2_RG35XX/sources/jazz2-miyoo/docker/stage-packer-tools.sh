#!/bin/sh
# Fills MiyooPackage/Tools/ with everything the player-facing converter needs. Run after
# docker/build-packer-win.sh and before building the release archive.
#
# EngineContent/ is jazz2-native's own Content directory (animation and metadata descriptions, and the
# compiled translations) - engine data, not game data, so it ships with the port. The converter hands it
# to AssetPacker as the "Content" half of a source layout; without it the prebaked archive is missing
# every engine-side asset override and the game loads a broken content set.
set -e
cd /work

TOOLS=MiyooPackage/Tools
mkdir -p "$TOOLS"

cp -f build-packer-win/Sources/Utilities/AssetPacker/AssetPacker.exe "$TOOLS/"

rm -rf "$TOOLS/EngineContent"
cp -r jazz2-native/Content "$TOOLS/EngineContent"
# The game loads only the compiled catalogs; the .po sources next to them are build inputs
find "$TOOLS/EngineContent/Translations" -name '*.po' -delete

echo "Staged:"
du -sh "$TOOLS"/* 
