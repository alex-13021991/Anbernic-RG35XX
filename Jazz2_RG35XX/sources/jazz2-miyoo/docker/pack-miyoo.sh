#!/bin/sh
# Builds the prebaked content tree the Miyoo Mini loads directly - no first-run conversion,
# no original game files on the device.
#
#   --target=console        writes a Content/ tree (Source.pak -> the prebaked package name),
#                           so RefreshCache() short-circuits on IsContentPrebaked()
#   --originals-only        only the episodes/levels the original game shipped
#   --skip-non-episode-levels  drops the Episodes/unknown pile
#   --video-downscale=2     re-encodes the cinematics into the cheap-to-decode container
#                           at half size (the Dreamcast path), which is what a 1.2 GHz A7 wants
set -e
cd /work

OUT="${1:-out-miyoo}"
rm -rf "$OUT"
./build-linux-sw/Sources/Utilities/AssetPacker/AssetPacker convert build-linux-sw "$OUT" \
    --target=console \
    --originals-only \
    --skip-non-episode-levels \
    --video-downscale=2

echo "=== TREE ==="
du -sh "$OUT"
du -sh "$OUT"/* 2>/dev/null

# The game loads only the compiled catalogs (see i18n.LoadFromFile in Main.cpp), the .po sources
# next to them are build inputs that would just sit on the SD card
find "$OUT/Translations" -name '*.po' -delete 2>/dev/null || true

echo "=== TRIMMED ==="
du -sh "$OUT"

# Music the level converter never sees.
#
# ConvertLevels only copies tracks that a surviving LEVEL references, so everything the engine
# asks for by name is missed: the main menu plays menu.j2b (and bonus2/bonus3.j2b behind a coin
# flip - MainMenu.cpp:454,460), and each cinematic plays a track named after itself
# (Cinematics.cpp:231). Without these the menu and the intro are silent, with nothing logged.
echo "Copying menu and cinematic music..."
for TRACK in menu bonus2 bonus3 intro ending; do
    SRC=$(ls /work/build-linux-sw/Source/ 2>/dev/null | grep -ix "$TRACK.j2b" | head -1)
    if [ -n "$SRC" ]; then
        cp -f "/work/build-linux-sw/Source/$SRC" "$OUT/Music/$TRACK.j2b"
        echo "  $SRC -> Music/$TRACK.j2b"
    else
        echo "  WARNING: $TRACK.j2b not found in Source"
    fi
done
