#!/bin/sh
cd "/mnt/SDCARD/Roms/PORTS/Games/Jazz Jackrabbit 2"
chmod +x ./jazz2

# Without this the engine resolves its config path from HOME, lands on /home/root/.config and
# fails - that path is on a read-only filesystem, so settings and save games are silently lost
# every run. launch_standalone.sh assigns HOME but does not export it, so it never reaches us.
export HOME="$PWD"

# Our patched libSDL2 first, then OnionOS's own. Same layering the Captain Claw port uses.
# /config/lib and .tmp_update/lib hold the SigmaStar libs (libmi_ao, libshmvar, ...) that the
# device libSDL2 needs. OnionOS puts them in the environment when launching from the menu, but
# not over SSH - listing them here makes the script work either way.
export LD_LIBRARY_PATH=".:./lib:/mnt/SDCARD/.tmp_update/lib/parasyte:/mnt/SDCARD/.tmp_update/lib:/config/lib:/customer/lib:$LD_LIBRARY_PATH"

# The Miyoo SDL2 only implements the "mmiyoo" backends (SigmaStar MI_GFX / MI_AO); without
# these SDL_Init falls through with "No available video device".
export SDL_VIDEODRIVER=mmiyoo
export SDL_RENDER_DRIVER=mmiyoo
# OpenAL is built with only its SDL2 backend, so audio rides the device SDL2 "mmiyoo" driver
# (SigmaStar MI_AO). OnionOS holds MI_AO exclusively through its own audioserver, so that has to
# be stopped first or MI_AO_SetPubAttr fails - the same fix the RetroArch ports use.
export SDL_AUDIODRIVER=mmiyoo
# See the file itself: without an explicit period_size OpenAL asks MI_AO for 882-sample frames
# and the output is badly distorted.
export ALSOFT_CONF="$PWD/alsoft.conf"
# The audioserver is stopped by launch_standalone.sh instead (KillAudioserver=1 in the .port).
# Do not source stop_audioserver.sh here as well: it also kills wpa_supplicant and udhcpc when
# they hold libpadsp.so, which drops WiFi - and doing that twice per launch is pointless.

# Window = the full 640x480 panel, internal render resolution = 320x240 (the build pins
# LevelHandler::DefaultWidth/Height there, and OnInitializeViewport caps the viewport to it).
# The direct tier sizes the CPU framebuffer to the internal resolution and lets the mmiyoo
# backend's MI_GFX blit stretch it 2x to the panel. Without this the window itself is 320x240
# and the game renders into the top-left corner of the screen, leaving stale pixels around it.
export JAZZ2_RESOLUTION=640x480

# Keep the CPU at its stock maximum instead of letting the governor scale down. Not an
# overclock - and it matters here, because the whole point of this build is an FPS number.
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || true

[ -f log.txt ] && mv -f log.txt log.prev.txt
# Steady 30 FPS. The 40 is deliberate and is NOT the frame rate being asked for.
#
# The page flip is FBIOPAN_DISPLAY, which the framebuffer driver holds until vblank, so the display
# can only ever show 60/N. The engine limiter, though, is purely time based: it sleeps until the
# frame has lasted 1/limit seconds (Application.cpp). Setting it to exactly 30 makes it release the
# frame at 33.33 ms - the very instant a vblank falls - so any jitter misses it and waits a whole
# extra period, giving 50 ms. Frames then alternate 33/50 and average out around 25 FPS, which is
# what the counter showed.
#
# Asking for 40 sets the target period to 25 ms instead: comfortably past 16.67 (so no frame can
# finish early enough to be shown at 60) and comfortably short of 33.33 (so the flip always lands
# on the second vblank rather than the third). The vblank does the locking, the limiter only stops
# frames arriving too early, and the result is a flat 30.
./jazz2 /max-fps:40 > log.txt 2>&1
EXIT_CODE=$?

{
    echo ""
    echo "=== jazz2 exited, code $EXIT_CODE (137=SIGKILL/OOM, 139=SIGSEGV) ==="
    echo "--- free ---"
    free -m
    echo "--- dmesg tail ---"
    dmesg | tail -40
} >> log.txt 2>&1

sync
