#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
mkdir -p "$WORK/logs"
(cd "$PORT" && sha256sum -c vendor/SHA256SUMS)
if [[ ! -x "$RG_TOOLCHAIN/bin/arm-linux-g++" ]]; then
    echo 'SDK missing. Obtain Miyoo GCC 7.3.0 from https://github.com/alex-13021991/RG35XX-Development and set RG_TOOLCHAIN to its extracted directory.' >&2
    exit 1
fi
export LD_LIBRARY_PATH="$RG_TOOLCHAIN/lib"
[[ $("$RG_TOOLCHAIN/bin/arm-linux-g++" -dumpversion) == 7.3.0 ]] || { echo 'Expected GCC 7.3.0'; exit 1; }
if [[ ! -f "$WORK/sysroot.ready" ]]; then
    [[ ! -e "$RG_SYSROOT" ]] || { echo 'Refusing to mix an existing unmarked sysroot'; exit 1; }
    mkdir -p "$RG_SYSROOT"
    tar xf "$PORT/vendor/garlicos-1.4.9-sysroot.tar.gz" -C "$RG_SYSROOT"
    touch "$WORK/sysroot.ready"
fi
if [[ ! -f "$WORK/openmpt.ready" ]]; then
    tar xf "$PORT/vendor/libopenmpt-0.5.34.tar.gz" -C "$WORK"
    (
        cd "$WORK/libopenmpt-0.5.34+release.autotools"
        export CC="$RG_TOOLCHAIN/bin/arm-linux-gcc" CXX="$RG_TOOLCHAIN/bin/arm-linux-g++"
        export CFLAGS="--sysroot=$RG_SYSROOT $CPU -O2"
        export CXXFLAGS="$CFLAGS"
        export LDFLAGS="--sysroot=$RG_SYSROOT -Wl,-rpath-link,$RG_SYSROOT/usr/lib,-rpath-link,$RG_SYSROOT/lib"
        export PKG_CONFIG_LIBDIR="$RG_SYSROOT/usr/lib/pkgconfig" PKG_CONFIG_SYSROOT_DIR="$RG_SYSROOT"
        export ZLIB_CFLAGS="-I$RG_SYSROOT/usr/include" ZLIB_LIBS="-lz"
        export MPG123_CFLAGS="-I$RG_SYSROOT/usr/include" MPG123_LIBS="-lmpg123"
        export OGG_CFLAGS="-I$RG_SYSROOT/usr/include" OGG_LIBS="-logg"
        export VORBIS_CFLAGS="-I$RG_SYSROOT/usr/include" VORBIS_LIBS="-lvorbis"
        export VORBISFILE_CFLAGS="-I$RG_SYSROOT/usr/include" VORBISFILE_LIBS="-lvorbisfile"
        ./configure --host=arm-miyoo-linux-uclibcgnueabi --prefix=/usr --with-sysroot="$RG_SYSROOT"           --disable-openmpt123 --disable-examples --disable-tests --disable-doxygen-doc           --with-mpg123 --with-ogg --with-vorbis --with-vorbisfile           --without-portaudio --without-portaudiocpp --without-pulseaudio --without-sndfile --without-flac           --disable-static --enable-shared
        make -j"$JOBS"
        make DESTDIR="$RG_SYSROOT" install
    ) > "$WORK/logs/openmpt.log" 2>&1
    touch "$WORK/openmpt.ready"
fi
