# Сборка в WSL Ubuntu 24.04

Сборка использует включённую исходную базу и отдельные патчи; сетевой загрузки игры нет. sources/upstream/ не изменяется.

## Зависимости

Python 3.12, Git, GNU patch, make, tar, g++ для теста UTF-8:

    sudo apt-get install python3 git patch make g++ tar

Кросс-компилятор: Miyoo GCC 7.3.0 ARM/uClibc, по умолчанию ~/RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0. [Тулчейны](../../Toolchains/).

Sysroot: архив 20261125T155126Z-sysroot.tar.gz из [RG35XX-Development/Sysroot](https://github.com/alex-13021991/RG35XX-Development/tree/main/Sysroot). Ожидаемый путь: ~/RG35XX/RG35XX-Development/Sysroot/20261125T155126Z-sysroot.tar.gz. SDK и sysroot не входят в этот порт.

Pillow нужен только для пересоздания шрифтов; готовые PNG включены.

## Сборка и упаковка

Из каталога SuperTux_RG35XX:

    sha256sum -c SOURCE-SHA256SUMS
    bash scripts/build.sh en
    bash scripts/build.sh ru
    bash scripts/test.sh
    python3 scripts/package.py --out "$PWD/dist"

Английский вариант содержит слой RG35XX; русский — тот же порт и следующий слой русификации. Скрипт подготовки копирует upstream, восстанавливает полноразмерные ресурсы, возвращает звуковые исправления ветки Miyoo и накладывает патчи.

Итог сверяется с sources/prepared-en-sha256.json или prepared-ru-sha256.json. Повторный запуск проверяет существующее дерево и не затирает вручную изменённые файлы. Для нового эксперимента задайте другой рабочий каталог:

    export SUPERTUX_WORK="$HOME/RG35XX/Work/supertux-build"
    export SDK="$HOME/RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0"
    export SYSROOT_ARCHIVE="$HOME/RG35XX/RG35XX-Development/Sysroot/20261125T155126Z-sysroot.tar.gz"
    bash scripts/build.sh ru

Переменные: SUPERTUX_WORK, SDK, SYSROOT, SYSROOT_ARCHIVE, JOBS. Используйте абсолютные пути. По умолчанию рабочий каталог — .work/, sysroot — .work/sysroot/, журналы — .work/logs/.

Параметры: GCC 7.3.0, -marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp -O2 -std=gnu++03. SDL 1.2, SDL_image, SDL_mixer и SDL_gfx берутся из GarlicOS. OpenGL отключён.

## Только подготовка дерева

    python3 scripts/prepare-source.py en
    python3 scripts/prepare-source.py ru

Патчи применяются GNU patch без fuzz. Часть исторических ресурсов содержит CRLF: режим --binary позволяет точно применить переход к UTF-8/LF в русском слое. Манифесты проверяют конечные байты.

## ABI

При наличии RG35XX-Development:

    python3 "$HOME/RG35XX/RG35XX-Development/Scripts/abi.py" .work/ru/source/supertux .work/sysroot

Если задан SUPERTUX_WORK, подставьте его пути. Проверка ABI не заменяет запуск на устройстве.

## Редактирование

Подготовленные исходники можно редактировать напрямую и собирать через make -f Makefile.rg35xx SDK=... SYSROOT=..., минуя проверку исходного манифеста. Для выпуска нужно перенести изменения в патч и обновить SHA-256.

Генератор шрифтов находится в подготовленном дереве:

    cd .work/ru/source
    python3 tools/generate_fonts.py

Требуется Pillow. Источник шрифта и лицензия — tools/font-source/. Обновляются только текстовые таблицы data/fonts/, оригинальная графика data/images/ не меняется.

## Проверка настоящего ввода в QEMU

После упаковки распакуйте полный установочный ZIP в отдельный каталог. Передайте путь к каталогу с supertux и data.pak:

    python3 .work/ru/source/tests/qemu-controls.py --qemu "$HOME/RG35XX/QEMU" --app /путь/Roms/PORTS/SuperTux --session supertux-controls-test

Тест управляет игрой через evdev-модель, проверяет неизменность привязок при прокрутке, захват A/B/X/Y, отмену, игнор плечевых кнопок, сброс, повторный запуск и удержание MENU в уровне с записью слота. Он создаёт отдельный сеанс QEMU и не обращается к физической приставке.

Установочные архивы содержат только data.pak; исходное дерево data/ сохраняется в комплекте исходников для редактирования. Упаковщик проверяет каждую запись по исходным байтам.
