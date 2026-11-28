#!/bin/sh
base=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$base" || exit 1
export HOME="$base/save"
export XDG_CONFIG_HOME="$HOME/config"
export XDG_DATA_HOME="$HOME/data"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_DATA_HOME"
export LD_LIBRARY_PATH="$base/lib:/usr/lib:/lib"
export SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=alsa
export SDL_NOMOUSE=1 SDL_NOMOUSEMOTION=1
width=640
if [ -f "$base/render-width.txt" ]; then
    IFS= read -r width < "$base/render-width.txt"
fi
width=$(printf '%s' "$width" | tr -d '\r')
case "$width" in
    320) export PVZ_RENDER_WIDTH=320 ;;
    400) export PVZ_RENDER_WIDTH=400 ;;
    480) export PVZ_RENDER_WIDTH=480 ;;
    640) export PVZ_RENDER_WIDTH=640 ;;
    800) export PVZ_RENDER_WIDTH=800 ;;
    *) export PVZ_RENDER_WIDTH=640 ;;
esac
printf 'RG35XX launcher: requested width=%s selected width=%s\n' "$width" "$PVZ_RENDER_WIDTH" > run.log
if [ ! -f main_mmp.pak ] && [ ! -f main.pak ]; then
    printf 'Game assets missing. See README_RU.txt and scripts/prepare-assets.py.\n' >> run.log
    exit 1
fi
./PvZ >> run.log 2>&1
