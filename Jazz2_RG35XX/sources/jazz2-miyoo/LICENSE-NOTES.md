# Licensing and third-party components

## This port

The engine is [jazz2-native](https://github.com/deathkiller/jazz2-native) by Dan R. (deathkiller),
licensed under the **GNU General Public License v3.0**. This port is derived from it, so it carries
the same licence — see `LICENSE`.

The corresponding source for the binary in a release is this repository: the port's changes live in
`patches/0001-miyoo-port.patch` against upstream commit `b8cba70`, and `docker/fetch-engine.sh`
reproduces the exact tree they apply to.

## What ships in a release

| Component | Version | Licence | Source |
|---|---|---|---|
| Jazz² Resurrection (`jazz2`) | `b8cba70` + `patches/` | GPL-3.0 | [deathkiller/jazz2-native](https://github.com/deathkiller/jazz2-native) |
| `AssetPacker.exe` | same | GPL-3.0 | the same tree; built by `docker/build-packer-win.sh` |
| `EngineContent/` | same | GPL-3.0 | the engine's own `Content/` — animation and metadata descriptions, compiled translations |
| OpenAL Soft (`libopenal.so.1`) | 1.21.1 | LGPL-2.1-or-later | [kcat/openal-soft](https://github.com/kcat/openal-soft); built unmodified by `docker/build-openal.sh` |
| libopenmpt (`libopenmpt.so.0`) | 0.5.34 | BSD-3-Clause | [lib.openmpt.org](https://lib.openmpt.org/libopenmpt/); built unmodified by `docker/build-openmpt.sh` |
| SDL2 (`libSDL2-2.0.so.0`) | Miyoo `mmiyoo` fork | Zlib | see below |
| `libstdc++.so.6` | GCC 8.3 (Miyoo toolchain) | GPL-3.0 with the GCC Runtime Library Exception | the Miyoo Mini toolchain |

OpenAL Soft is **LGPL** and ships as a shared library, unmodified, exactly so it can be replaced:
`docker/build-openal.sh` is the complete recipe, and dropping a different build into the port's
`lib/` directory is all it takes.

### SDL2

The device's own SDL2 only implements the SigmaStar `mmiyoo` backends, and its stock build has
defects this port needs fixed. The library shipped here is the one built for the
[Captain Claw Miyoo port](https://github.com/ArticlessCZ-Coder/captain-claw-miyoo), which publishes
its changes as `patches/sdl2-miyoo-openclaw.patch` against upstream SDL. SDL2 is Zlib-licensed.

## What never ships

**Jazz Jackrabbit 2's game data.** The graphics, levels, tilesets, music and cinematics belong to
their copyright holders. None of it is in this repository, in any release, or in anything the
converter is distributed with — the converter reads the copy the player already owns and writes the
result on the player's own machine.
