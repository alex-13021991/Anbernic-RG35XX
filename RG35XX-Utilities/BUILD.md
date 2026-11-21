# RG35XX utilities 0.1.2

Two C99 applications for original RG35XX, GarlicOS 1.4.9, SDL 1.2.
The UI is shared source; the installed apps and launchers are independent.

## Build

Use the original Miyoo **uClibc** SDK, not a newer musl/H700 SDK.
Windows build used Zig 0.13.0 as Clang/LLD, with SDK headers, startup objects
and dynamic libraries. No Zig libc is linked into the target binaries.

    py -3.14 build.py --sdk C:/sdk/miyoo --zig C:/zig/zig.exe

Downloads / primary references:
- https://ziglang.org/download/0.13.0/zig-windows-x86_64-0.13.0.zip
- https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z
- https://github.com/edemirkan/rg35xx-toolchain
- https://github.com/IonAgorria/rg35xx-mtp-server

Extract the SDK with symlinks intact. On Windows, links unpacked as text must
be resolved inside the SDK before linking. Output: build/usb-manager and
build/screenshot-viewer. ARMv7 Cortex-A9, softfp, /lib/ld-uClibc.so.0.
SDL 1.2, SDL_image and SDL_ttf remain system dependencies.

From the project directory, package the rebuilt binaries:

    py -3.14 package.py

The installer ZIPs and source ZIP are written to the sibling `outputs`
directory. Packaging reuses the existing app icons and needs only Python's
standard library.

The bundled launchers preload the user's existing DinguxCommander j2k.so;
keyboard mapping matches that program's GarlicOS build (w/s/q/d, a/b/x/y,
h/j/l/k, u). Resources are local to each app directory. No firmware edits.

## Tests and scope

tests/test_usb.py runs the real shell save helper against isolated fixture
directories in Git Bash on Windows; mount/sync and one injected failing touch
are mocked. It exercises all 25 transitions, validation, failure rollback,
locking and interrupted-save recovery. It never mounts the host's disks.

tests/test_core.c compiles the real gallery.c and core.h for wasm32-wasi and
runs against disposable PNG fixtures in Node WASI. WASI lacks realpath, so
the fixture shim accepts canonical absolute fixture paths only. The Windows
WASI readdir compatibility implementation is supplied by run-wasi.cjs;
stat/open/unlink still use the WASI filesystem. Tests do NOT validate Linux
symlink resolution, kernel USB state or target SDL behavior.

Example WASI compile:

    zig cc -target wasm32-wasi -D_GNU_SOURCE -O0 -g -Wl,-z,stack-size=1048576 -include tests/wasi_compat.h tests/test_core.c src/gallery.c tests/wasi_compat.c -o build/test-core.wasm
    python tests/fixtures.py
    node tests/run-wasi.cjs build/test-core.wasm tests/fixtures

verify.py checks ELF ABI, imports against the SDK, Cyrillic glyph coverage
and the width of control hints. Its default resource paths refer to the local
build workspace; adapt `prior` when reproducing these asset checks elsewhere.

No execution on RG35XX hardware or target SDL/framebuffer has been performed.
See README_RU.txt for installation, limits and first-device checks.
