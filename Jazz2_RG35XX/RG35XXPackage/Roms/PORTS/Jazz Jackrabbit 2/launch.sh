#!/bin/sh
cd "$(dirname "$0")" || exit 1
mkdir -p Saves Logs || exit 1
if [ -f Logs/log.txt ]; then mv -f Logs/log.txt Logs/log.prev.txt || exit 1; fi
exec >Logs/log.txt 2>&1
if [ ! -s Content/Prebaked.pak ] || [ ! -s Content/Prebaked.version ]; then
    echo 'Скопируйте подготовленную папку Content рядом с файлом jazz2.'
    exit 2
fi
export HOME="$PWD/Saves" XDG_CONFIG_HOME="$PWD/Saves" XDG_DATA_HOME="$PWD/Saves"
export SDL_VIDEODRIVER=dummy JAZZ2_RESOLUTION=640x480
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LANG=ru_RU.UTF-8 ALSOFT_CONF="$PWD/alsoft.conf"
./jazz2 /config "$PWD/Saves/Jazz2.config" /max-fps:30 "$@"
rc=$?
echo "Код выхода Jazz2: $rc"
exit "$rc"
