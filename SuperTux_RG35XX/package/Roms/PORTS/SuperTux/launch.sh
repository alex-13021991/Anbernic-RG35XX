#!/bin/sh
cd "$(dirname "$0")" || exit 1
mkdir -p Saves Logs || exit 1
if [ -f Logs/run.log ]; then mv -f Logs/run.log Logs/run.prev.log || exit 1; fi
exec >Logs/run.log 2>&1
export HOME="$PWD/Saves"
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=alsa SDL_NOMOUSE=1
export LD_LIBRARY_PATH="/usr/lib:/lib"
./supertux "$@"
rc=$?
echo "SuperTux exit: $rc"
exit "$rc"
