#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
if [[ -x "$RG_TOOLCHAIN/bin/$TRIPLE-g++" ]]; then
    echo "Using existing toolchain: $RG_TOOLCHAIN"; exit 0
fi
archive=${JAZZ2_TOOLCHAIN_ARCHIVE:-"$WORK/downloads/env_toolchain.7z"}
if [[ ! -f "$archive" ]]; then
    mkdir -p "$(dirname "$archive")"
    curl -fL --retry 3 https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z -o "$archive.part"
    mv "$archive.part" "$archive"
fi
echo "70d4ea2d477f931eedbc7e049cddd672098a92c7b9e5672bd90b65143db73c70  $archive" | sha256sum -c -
[[ ! -e "$RG_TOOLCHAIN" ]] || { echo "Refusing to overwrite: $RG_TOOLCHAIN" >&2; exit 1; }
staging=$(mktemp -d "${TMPDIR:-/tmp}/jazz2-toolchain.XXXXXX")
7z x -y -o"$staging" "$archive" '-x!miyoo/arm-miyoo-linux-uclibcgnueabi/sysroot' '-x!miyoo/usr' > "$staging/extract.log"
mkdir -p "$(dirname "$RG_TOOLCHAIN")"
mv "$staging/miyoo" "$RG_TOOLCHAIN"
echo "Toolchain: $RG_TOOLCHAIN"
