# Jazz Jackrabbit 2 for the Miyoo Mini

A port of [Jazz² Resurrection](https://github.com/deathkiller/jazz2-native) — an open-source
reimplementation of the Jazz Jackrabbit 2 engine — to the Miyoo Mini and Miyoo Mini Plus running
OnionOS.

**You need your own copy of Jazz Jackrabbit 2.** This repository ships the engine and the tools,
never the game. A converter is included that turns your installation into the data the port loads.

The game runs at a steady 30 FPS on the device's 640×480 panel, with sound effects, module music
and the intro and ending cinematics.

---

## Installing

1. Download the SD card archive from the [latest release](../../releases/latest) and unpack it
   into the root of your SD card. It adds:

   ```
   Roms/PORTS/Games/Jazz Jackrabbit 2/     the engine and its launcher
   Roms/PORTS/Shortcuts/Action/            the entry in the PORTS menu
   Tools/                                  the converter (runs on your PC, not on the device)
   ```

2. Convert your copy of the game **on a Windows PC**: drag your Jazz Jackrabbit 2 folder onto
   `Tools\Convert Jazz Jackrabbit 2.bat`. It writes a `Content` folder next to itself; this takes
   a few seconds.

   Any release of the game works — the original CD, the Gathering of Developers re-release, GOG,
   Steam. Point the converter at the folder that holds `Anims.j2a` and `Jazz2.exe`. The Shareware
   Demo is not enough; the converter needs the full game's data.

3. Copy that `Content` folder into `Roms/PORTS/Games/Jazz Jackrabbit 2/` on the SD card.

4. Eject the card, and start **Jazz Jackrabbit 2** from the PORTS menu.

The `Tools` folder is only needed for the conversion. Delete it from the card afterwards if you
want the space back.

### Not on Windows?

The converter is `AssetPacker`, a tool that is part of the engine and builds on Linux and macOS
too. Build it from the upstream source and run:

```
AssetPacker <staging directory> Content --target=console --originals-only \
    --skip-non-episode-levels --video-downscale=2
```

where the staging directory holds your game files in `Source/` and this port's `EngineContent`
as `Content/`. Then copy `menu.j2b`, `bonus2.j2b`, `bonus3.j2b`, `intro.j2b` and `ending.j2b` from
the game into `Content/Music/` — the converter only carries over music a level references, so the
menu's and the cinematics' tracks have to be added by hand. `docker/pack-miyoo.sh` in this
repository does exactly this and is the shortest thing to copy from.

---

## Controls

| Button | In game | In menus |
|---|---|---|
| D-pad | Move | Navigate |
| **B** | Jump | Back |
| **A** | Run | Confirm |
| **Y** | Fire | — |
| **X** | Change weapon | — |
| **START** | Pause menu | Confirm |
| **SELECT** | Menu | Back |
| **MENU** | Quit to the PORTS menu | Quit |

Menus follow the console's own convention — A confirms and B goes back — while the game itself
keeps the engine's layout, where B is the jump button under your thumb.

---

## What works

- The full single-player campaign: all six original episodes, all their levels
- Jazz, Spaz and Lori
- Sound effects and music (the `.j2b` module tracks, through libopenmpt)
- The intro and ending cinematics, at half resolution
- Saving, settings, and the in-game menu

## What does not

- **Multiplayer of any kind.** Split-screen, LAN and online are all out
- **Custom levels and episodes.** The converter deliberately keeps only what the original game
  shipped; a level from elsewhere is not converted and will not appear
- **Textured backgrounds.** JJ2's warped perspective sky is a per-pixel effect that cost more than
  a third of the frame on this hardware, so the layer draws flat instead. It is the one visible
  compromise in the port
- **The rendering is not pixel-exact.** The internal resolution is 304×228, upscaled by the
  display hardware, and the lighting is computed with a NEON approximation that is a shade off the
  reference

---

## Reporting a problem

`Roms/PORTS/Games/Jazz Jackrabbit 2/log.txt` holds the last run and `log.prev.txt` the one before.
Attach the log and say which level it happened in.

If the game refuses to start with *"Prepared content … was built for cache version …"*, the
`Content` folder was made by an older converter than the engine expects: run the converter from
this release again.

---

## Building it yourself

The Windows converter builds from this repository alone:

```bash
sh docker/fetch-engine.sh                    # clone the engine at the pinned commit, apply patches/
docker build -t jazz2-mingw -f docker/Dockerfile.mingw docker/
W="docker run --rm -v $PWD:/work jazz2-mingw"
$W sh docker/build-packer-win.sh
$W sh docker/stage-packer-tools.sh           # fills MiyooPackage/Tools/
```

The device binary needs three things this repository cannot ship, all of them large binaries or
other people's work:

- the **Miyoo Mini toolchain** (GCC 8.3 / glibc 2.28) from
  [shauninman/union-miyoomini-toolchain](https://github.com/shauninman/union-miyoomini-toolchain),
  as `toolchain/miyoomini-toolchain.tar.xz` in the build context. A Debian armhf cross-compiler
  will build and run under qemu but produces a binary the device cannot start — it links glibc 2.36
- the **patched libSDL2** in `device-libs-upload/`, whose changes the Captain Claw port publishes as
  `patches/sdl2-miyoo-openclaw.patch`
- its **headers** in `sdl2-miyoo-src/`, so headers and the deployed library are the same version

With those in place, `docker/Dockerfile.miyoo-tc` builds the toolchain image and the rest follows:

```bash
docker build -t jazz2-miyoo-tc -f docker/Dockerfile.miyoo-tc <that build context>
R="docker run --rm -v $PWD:/work jazz2-miyoo-tc"
$R sh docker/build-openal.sh                 # OpenAL Soft, SDL2 backend only
$R sh docker/build-openmpt.sh                # libopenmpt, for the .j2b module music
$R sh docker/configure-miyoo-device.sh
$R cmake --build /work/build-miyoo-device --parallel
```

The engine is **not vendored**. `patches/0001-miyoo-port.patch` holds the port's changes against
upstream commit `b8cba70`, and `docker/fetch-engine.sh` reproduces the tree.

### The decisions behind the port

Worth knowing if you are looking at the patch, or porting something similar to this hardware:

- **The rasterizer is fill bound, not draw-call bound.** A level with *fewer* pixels and *fewer*
  draws ran three times slower than a heavier one, which is what pointed the work at per-pixel
  cost. Cutting the pixel count by three quarters ran 2.9× faster
- **Textured backgrounds were the single biggest win** — 19 → 34 FPS in the worst level. JJ2's
  warped sky is a procedural per-pixel effect that shades close to a full screen through the
  general sampled path
- **304×228, not 320×240.** 5 % off each axis roughly halved the frame time, far more than the
  10 % fewer pixels explains. The framebuffer crossing the L2 cache size is the likely reason —
  a hypothesis, not something measured
- **The frame limiter is set to 40, and the result is a steady 30.** `FBIOPAN_DISPLAY` blocks
  until vblank, so the panel can only show 60/N. Asking for exactly 30 releases the frame at
  33.33 ms, right on a vblank boundary, so jitter misses it and the frame waits a whole extra
  period; frames then alternate 33/50 ms and average about 25. A 25 ms target lands safely
  between the two boundaries and every frame catches the second vblank
- **The conversion cannot run on the device.** It fits in memory (~33 MB) but crashes with
  SIGSEGV, which is why the converter is a PC-side tool and not a first-run step

`CHANGELOG.md` has the per-release detail, including the measurements each change is based on.

---

## Licensing

The engine is **GPL-3.0**, so this port is too. See `LICENSE` for the terms and `LICENSE-NOTES.md`
for the third-party components and where their sources are.

The game data is **not** covered by any of that. It belongs to its copyright holders, it is not in
this repository or in any release, and every player supplies their own.
