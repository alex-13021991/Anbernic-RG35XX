#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
mkdir -p "$WORK/logs"
(cd "$VENDOR" && sha256sum -c SHA256SUMS)
[[ -x "$RG_TOOLCHAIN/bin/$TRIPLE-g++" ]] || {
    echo 'Toolchain missing. Run: bash scripts/setup-toolchain.sh (or set RG_TOOLCHAIN).' >&2; exit 1;
}
export LD_LIBRARY_PATH="$RG_TOOLCHAIN/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
[[ $("$TRIPLE-g++" -dumpversion) == 7.3.0 ]] || { echo 'Expected Miyoo GCC 7.3.0' >&2; exit 1; }
if [[ ! -f "$WORK/sysroot.ready" ]]; then
    [[ ! -e "$RG_SYSROOT" ]] || { echo "Refusing to mix an existing sysroot: $RG_SYSROOT" >&2; exit 1; }
    mkdir -p "$RG_SYSROOT"
    tar xf "$VENDOR/garlicos-1.4.9-build-sysroot.tar.gz" -C "$RG_SYSROOT"
    touch "$WORK/sysroot.ready"
fi
if [[ ! -f "$RG_SYSROOT/usr/lib/libopenmpt.so" ]]; then
    tar xf "$VENDOR/libopenmpt-0.5.34.tar.gz" -C "$WORK"
    cd "$WORK/libopenmpt-0.5.34+release.autotools"
    export PKG_CONFIG_LIBDIR="$RG_SYSROOT/usr/lib/pkgconfig" PKG_CONFIG_SYSROOT_DIR="$RG_SYSROOT"
    export CC="$RG_TOOLCHAIN/bin/$TRIPLE-gcc" CXX="$RG_TOOLCHAIN/bin/$TRIPLE-g++"
    export CFLAGS="--sysroot=$RG_SYSROOT $CPU -O2" CXXFLAGS="--sysroot=$RG_SYSROOT $CPU -O2"
    export LDFLAGS="--sysroot=$RG_SYSROOT -Wl,-rpath-link,$RG_SYSROOT/usr/lib,-rpath-link,$RG_SYSROOT/lib"
    export ZLIB_CFLAGS="-I$RG_SYSROOT/usr/include" ZLIB_LIBS="-lz"
    ./configure --host="$TRIPLE" --prefix=/usr --with-sysroot="$RG_SYSROOT" --disable-openmpt123 \
        --disable-examples --disable-tests --disable-doxygen-doc --without-mpg123 --without-ogg \
        --without-vorbis --without-vorbisfile --without-portaudio --without-portaudiocpp \
        --without-pulseaudio --without-sndfile --without-flac --disable-static --enable-shared \
        2>&1 | tee "$WORK/logs/openmpt-configure.log"
    make -j"$JOBS" 2>&1 | tee "$WORK/logs/openmpt-build.log"
    make DESTDIR="$RG_SYSROOT" install
fi
