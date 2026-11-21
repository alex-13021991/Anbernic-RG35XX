#!/bin/sh
app_dir=$(CDPATH= cd "$(dirname "$0")/SystemInfoRU" && pwd) || exit 1
cd "$app_dir" || exit 1
if [ -f app.log ]; then mv -f app.log app-previous.log 2>/dev/null; fi
if ( : >>app.log ) 2>/dev/null; then exec >>app.log 2>&1; fi
printf 'System Info RU 1.0.0 | GarlicOS 1.4.9\nDirectory: %s\n' "$app_dir"
export SDL_NOMOUSE=1
LD_PRELOAD="$app_dir/j2k.so" ./system-info
rc=$?
printf 'Exit: %s\n' "$rc"
exit "$rc"

