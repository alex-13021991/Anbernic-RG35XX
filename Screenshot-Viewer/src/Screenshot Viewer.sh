#!/bin/sh
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/ScreenshotViewer" && pwd) || exit 1
cd "$APP_DIR" || exit 1
if [ -f app.log ]; then mv -f app.log app-previous.log; fi
exec >app.log 2>&1
echo "Screenshot Viewer 0.1.5 — GarlicOS 1.4.9"
export SDL_NOMOUSE=1
LD_PRELOAD="$APP_DIR/j2k.so" ./screenshot-viewer
rc=$?
echo "Exit: $rc"
exit "$rc"
