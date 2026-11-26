# Changelog

Versions refer to the **Miyoo Mini port**, not to Jazz² Resurrection itself. The engine is
`jazz2-native` (upstream `deathkiller/jazz2-native`); this repository carries the port work on
top of it — the build environment, the asset pipeline and a handful of source changes that are
listed per release below.

Version numbers track *releases*, not builds. Work accumulates under Unreleased for as long as
it takes; a dozen test builds still add up to one version bump.

## 0.5.0 — 2026-08-22 — a port other people can install

Nothing about how the game plays changed. This release is the part that was missing for anyone
other than the author to use it: a converter that runs on a normal PC, a content tree that says
what it was built against, and the licensing the GPL requires.

### The converter

Converting on the device was never an option — it needs the original game files on the SD card,
and it crashed with SIGSEGV even though the memory fits. So `AssetPacker` is now cross-built for
Windows and shipped with the release, wrapped in a drag-and-drop batch file:

- **`AssetPacker.exe`**, built with mingw-w64 in `docker/build-packer-win.sh`. Statically linked,
  so it is one file with nothing to install
- **`Convert Jazz Jackrabbit 2.bat`**, which stages the layout the packer wants — the game folder
  as `Source` (a junction, so nothing is copied), this port's `EngineContent` as `Content` — runs
  the conversion, and adds the five music tracks the level converter never sees
- Verified against the Linux pipeline: the `Prebaked.pak` the two produce from the same
  installation is **byte-identical**

Four things had to be fixed in the build to get there, all of them in
`patches/0001-miyoo-port.patch`:

- `NCINE_BUILD_ASSET_PACKER_CROSS`, because the packer is skipped when cross-compiling
- the toolchain has to call itself `AMD64`; `cmake/ncine_imported_targets.cmake` compares
  `CMAKE_SYSTEM_PROCESSOR` against that exact string to pick the prebuilt library set, and with
  `x86_64` it silently chose the 32-bit one
- `-municode` on the AssetPacker target only — its Windows entry point is `wmain()`, and putting
  the flag in the toolchain instead breaks CMake's own compiler probes
- mingw's static `libz.a` in place of the prebuilt MSVC `zlib.lib`, which is an import library for
  a DLL the player would not have

### Prebaked content now says what it was built for

A prepared content tree bypasses the startup cache verification entirely — there are no original
files on the device to re-check. That left a tree built by an older converter silently loadable by
a newer engine, which shows up as missing or wrong assets rather than as an error.

The converter now writes `Content/Prebaked.version` recording the cache version and event count
the tree was built against, and the game refuses a tree that does not match, naming both numbers
and telling the player to convert again. A tree with no such file at all is refused the same way.
The refusal is the same state a missing `Anims.j2a` produces: the main menu drops its play items
and says what is wrong, rather than the game running on data it cannot read correctly.

### Documentation and licensing

- `README.md` — install, controls, what works and what does not, and how to build it
- `LICENSE` (GPL-3.0, from the engine) and `LICENSE-NOTES.md` — every third-party component, its
  licence and where its source is. OpenAL Soft is LGPL and ships as a replaceable shared library
- `patches/0001-miyoo-port.patch` regenerated: 438 lines across 17 files, verified to apply
  cleanly to upstream `b8cba70`

## 0.4.0 — 2026-08-20 — playable frame rate everywhere

Carrotus levels went from ~15 FPS to a steady 30. The frame rate no longer swings, and the menu
follows the console's own button convention.

### Performance

The rasterizer is **fill bound** — a quarter of the pixels ran 2.9× faster, which is what finally
aimed the work. Three changes, in order of what they were worth:

- **Textured backgrounds are off on this target** (`SupportsTexturedBackground(Circle)` in
  TileMap.cpp). JJ2's warped perspective sky is a per-pixel procedural effect: on a CPU rasterizer
  it shades close to a full screen through the general sampled path, calling a transpiled fragment
  per pixel. In `carrot1` it alone cost ~27 ms a frame. The layer still draws, through the plain
  tile renderer, just without the warp. **19 → 34 FPS**, and `castle1` unaffected
- **Internal resolution 320×240 → 304×228.** Only 5 % off each axis, but it roughly halves the
  frame time — 29.3 ms → 17.6 ms in `carrot1`. The jump is far larger than the 10 % pixel
  reduction explains; the framebuffer crossing the L2 cache size (150 kB → 139 kB) is the likely
  reason, though that is a hypothesis, not something measured
- **NEON variant of `combineLightingScanline`.** ARMv7 NEON has no vector divide, so the /255 and
  ×255 are folded out algebraically; the result is deliberately not bit-identical to the scalar
  sibling (documented at the code). Worth ~10 ms in `carrot1`, nothing in `castle1` — which is why
  it was wrongly dismissed in 0.3.0's notes

Measured on the device, `carrot1` ("Tossed Salad"):

| | frame time | FPS |
|---|---|---|
| 0.3.0 | 51–55 ms | ~15 |
| + NEON lighting | 42–44 ms | ~17 |
| + textured backgrounds off | ~29 ms | ~34 |
| + 304×228 | ~17.6 ms | ~57 |

### Fixed

- **Frame rate no longer swings between 60 and 30.** The page flip is `FBIOPAN_DISPLAY`, which the
  framebuffer driver holds until vblank, so only 60/N can ever be displayed - a frame landing just
  over 16.67 ms misses its vblank and waits for the next, and alternating 60/30 reads far worse
  than a constant 30. Capped with `/max-fps:40`, which sounds wrong and is not: the engine limiter
  is purely time based, so asking for exactly 30 releases each frame at 33.33 ms, the very instant
  a vblank falls, and any jitter misses it and costs a whole extra period — that beat averaged out
  at 25 FPS. A 25 ms target sits clear of both edges and lets the vblank do the locking
- **Menus follow the console's convention: A confirms, B goes back.** Gameplay is untouched
  (A still runs, B still jumps) because `FetchNavigation` is used only by menus and cinematics.
  Upstream's "Jump also confirms" rule is dropped here, since Jump is B and B now means back

### Known issues

- Config and control bindings are cached in `Jazz2.config`; changing built-in defaults requires
  deleting it
- The game aborts on exit (`free(): invalid pointer`) inside the device SDL2's video teardown.
  No effect on play or saved data
- On-device asset conversion crashes, so a PC-side packer is required for distribution

## 0.3.0 — 2026-08-19 — music, and a steady 30 FPS

Music plays. The frame rate no longer collapses in lit scenes. First build with no benchmark
instrumentation compiled in.

### Added

- **Music**, through libopenmpt 0.5.34 (0.6+ needs C++17, which GCC 8.3 cannot give). Built by
  `docker/build-openmpt.sh` with every optional dependency off, so it needs only zlib — which is
  not optional here: the compressed `.j2b` variant JJ2 uses is inflated through it
- The menu and cinematic tracks. `ConvertLevels` only copies music a surviving *level* references,
  so everything the engine asks for by name — `menu.j2b`, `bonus2`/`bonus3.j2b` (MainMenu.cpp) and
  a track named after each cinematic (Cinematics.cpp) — was silently missing. `pack-miyoo.sh`
  copies them explicitly now

### Performance

**The fire in Dungeon Dilemma dropped the game to 20 FPS. It is steady 30 FPS now.**

The cost was not the fire's animation or its particles — it was the light it casts. The software
tier builds a CPU lightmap and splats every light emitter into it texel by texel
(`CombineRenderer::PrepareSoftwareLighting`), and a torch has a large radius. The resolution of
that map is chosen per platform, and the Miyoo was falling into the *desktop* branch despite being
slower than the PSP already listed in the console one. It now uses the same quarter-resolution map:
4 800 texels instead of 19 200 at this port's 320×240.

Measured on the device, worst case at the fire:

| | before | after |
|---|---|---|
| `visit` | 10.1 ms | **2.2 ms** |
| `draw` | 24.6 ms | **18.5 ms** |
| CPU work per frame | 35.6 ms | **21.9 ms** |
| frame rate | fell to 20 FPS | **29.8–30.0 FPS** |

`visit` is in that table because `OnDraw` — where the lightmap is prepared — runs during the scene
traversal, not during rasterization. That is also why the spike looked like a scene-graph problem.
It was not: the node counter peaks at 8–11 nodes.

### Fixed

- `WaitForVerify()` spun on a non-atomic `_flags` written by the parallel init thread; at -O3 with
  LTO the compiler may hoist that load out of the loop. Closed, though see below — it was not the
  cause of the failure it was found while chasing

### Not done

- **60 FPS is out of reach without deeper work.** ~21 ms of CPU per frame against a 16.7 ms budget;
  what is left is the rasterizer itself, not lighting
- `-mthumb` was measured against `-marm` on an identical scene: 12.479 ms vs 12.523 ms, i.e. noise.
  Kept only because the binary is 22 % smaller
- **Starting a level from the command line (`jazz2 <level>.j2l`) dies with SIGSEGV**, right after
  "Waiting for content verification". Unrelated to launching from PORTS, which is unaffected, but
  it blocks self-service benchmarking. Cause unknown
- HUD is still not tuned for 320×240

## 0.2.0 — 2026-08-19 — sound effects

Audio works. Controls are complete. Music is still missing.

### Added

- **Sound effects**, through OpenAL Soft 1.21.1 built with only its SDL2 backend, so it rides
  the device's own SDL2 (`mmiyoo` → SigmaStar MI_AO) instead of needing ALSA, which the device
  has no userspace library for. Built by `docker/build-openal.sh`
- `KillAudioserver=1` in the `.port` — OnionOS holds MI_AO exclusively
- `alsoft.conf` for the cheap-resampler settings

### Fixed

- **Distorted audio and a crash after the intro, which were one bug.** The device's SDL2 sizes
  its mix buffer as `samples * 2 * channels` — two bytes per sample, hardcoded — and always
  programs MI_AO for 16-bit, but never forces `spec.format` to agree. OpenAL asks for 32-bit
  float and passes `SDL_AUDIO_ALLOW_ANY_CHANGE`, so SDL kept float: it wrote twice the bytes the
  buffer holds (heap corruption, surfacing as `free(): invalid pointer` and SIGABRT one scene
  later) while MI_AO read float bytes as 16-bit PCM (the distortion). Our OpenAL build now forces
  `AUDIO_S16SYS` and disallows format renegotiation
- **Period size** rounded up to a power of two (882 → 1024); MI_AO takes SDL's `spec.samples`
  straight through as `u32PtNumPerFrm` and only clocks a power-of-two frame correctly
- **START did nothing.** `ControlScheme.cpp` makes Return mean Fire as well as its binding, and
  `BeginSection` tests Fire before Menu — so START confirmed the highlighted item instead of
  opening the menu. The shortcut is dropped on this target; Y confirms
- SELECT also bound to Menu
- Menu and cinematics now honour the 320×240 logical resolution (they capped their viewports
  with their own copies of `DefaultWidth`, so they were rendering at 540×405)
- Config and saves persist — the engine resolved its path from `HOME`, landed on read-only
  `/home/root/.config` and lost everything each run
- The PORTS shortcut appeared as `.notfound`: OnionOS checks `GameDataFile` with
  `find -iname`, which matches a basename only, so the path form could never match

### Known issues

- No music — `.j2b` are modules and need libopenmpt
- The cost of audio is not measured yet: the 30 FPS seen in logs is vsync (`MaxFps` defaults to
  `UseVsync`, so a frame over 16.7 ms locks to 60/2), not load
- Changing default bindings requires deleting `Jazz2.config`, which persists them

## 0.1.0 — 2026-08-19 — first playable

Runs on real hardware, reaches gameplay, responds to the controls. No audio yet.

**Engine base:** upstream `b8cba70` (2026-08-13).

### Works

- Software rasterizer (`NCINE_PREFERRED_RHI=Software`), no OpenGL anywhere in the build
- **320×240 internal resolution, presented to the 640×480 panel** — the direct tier sizes the
  CPU framebuffer to the logical resolution and the mmiyoo backend's `MI_GFX_BitBlit` stretches
  it 2×, so the rasterizer never fills more than 76 800 pixels
- RGB565 framebuffer (`NCINE_RHI_USE_FB16=ON`), ~20–27 % cheaper than RGBA8888
- Prebaked content tree — no first-run conversion on the device, no original game files deployed
- Full campaign: 7 episodes plus Christmas Chronicles, cinematics, tilesets, music files
- Controls on the console's buttons, with MENU as an unconditional exit

### Measured on the device (Cortex-A7 @ 1.2 GHz, `performance` governor)

| | |
|---|---|
| Level `castle1` | **33 FPS** (measured at 320×240 without the panel upscale) |
| Intro cinematic | 59 FPS |
| Main menu | 19 FPS |
| Peak RSS | ~41 MB of the ~100 MB the system reports |

### Port changes to `jazz2-native`

- `LevelHandler.h`, `Cinematics.h`, `LoadingHandler.h`, `InGameMenu.h`, `MainMenu.h` — logical
  viewport size overridable at build time via `JAZZ2_LOGICAL_WIDTH`/`_HEIGHT`. Each class caps
  its own viewport, so all five have to agree or the menu renders at a different resolution
  than levels
- `Main.cpp` — `JAZZ2_RESOLUTION` env override for the window size; MENU key quits the game
- `SdlGfxDevice.h` — `isMainWindow()` accepts window ID 0. The mmiyoo backend sets only the
  mouse focus, so every key event it posts carries ID 0 and was being discarded as "not ours",
  which left the console with no working input at all
- `ControlScheme.cpp` — device key bindings. The advertised gamepad is a stub
  (`MMIYOO_JoystickUpdate()` is empty), so buttons arrive as key events
- `Utf8.cpp` — GCC 8.3 cannot deduce `StaticArray(const T(&)[size])` from a braced-init-list.
  This was the *only* C++17 problem in the whole build

### Not yet

- **No audio** — built with `NCINE_WITH_AUDIO=OFF`; jazz2 needs OpenAL, which the device has not
- HUD scale is not tuned for 320×240
- Level `carrot1` hangs at "Waiting for content verification", cause unknown
- `FrameTimer.cpp` still carries the benchmark logging patch, deliberately
