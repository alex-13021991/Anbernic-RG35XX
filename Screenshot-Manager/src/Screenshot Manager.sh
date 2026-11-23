#!/bin/sh
app_dir=$(CDPATH= cd "$(dirname "$0")/ScreenshotManager" && pwd) || exit 1
cd "$app_dir" || exit 1
export SDL_NOMOUSE=1
export LD_PRELOAD="$app_dir/j2k.so"
exec ./manager
