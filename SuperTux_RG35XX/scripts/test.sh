#!/usr/bin/env bash
set -euo pipefail
PORT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${SUPERTUX_WORK:-"$PORT/.work"}
mkdir -p "$WORK/logs"
(cd "$PORT" && sha256sum -c SOURCE-SHA256SUMS >"$WORK/logs/source-integrity.txt")
python3 "$PORT/scripts/prepare-source.py" en --work "$WORK"
python3 "$PORT/scripts/prepare-source.py" ru --work "$WORK"
g++ -std=c++11 "$WORK/ru/source/tests/text-ru.cpp" -o "$WORK/test-text-ru"
"$WORK/test-text-ru"
python3 "$PORT/scripts/verify-localization.py" --work "$WORK"
sh -n "$PORT/package/Roms/PORTS/SuperTux.sh"
sh -n "$PORT/package/Roms/PORTS/SuperTux/launch.sh"
echo 'Контрольные суммы, UTF-8, ресурсы, структура уровней и скрипты: OK'
