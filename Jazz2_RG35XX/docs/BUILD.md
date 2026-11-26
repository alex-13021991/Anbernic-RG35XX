# Сборка и восстановление проекта

## Что требуется извне

В репозитории находятся исходники, патчи, целевой sysroot и зависимости проекта. Отдельно устанавливаются инструменты ОС и Miyoo GCC 7.3.0: сам тулчейн намеренно не включён. Оригинальные ресурсы Jazz Jackrabbit 2 нужны только для игры и проверки с уровнями, но **не для компиляции**.

Для работы достаточно одного клона или ZIP этого репозитория. Каталоги прежнего компьютера `jazz2-native`, `jazz2-miyoo`, `RG35XX-Development`, `Private-Jazz2` и старый кеш не нужны.

## 1. WSL Ubuntu 20.04 x86-64

В PowerShell Windows при отсутствии WSL:

```powershell
wsl --install -d Ubuntu-20.04
wsl -d Ubuntu-20.04
```

Дальнейшие команды выполняются **в Bash Ubuntu**, а не в PowerShell:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git pkg-config \
  python3 p7zip-full curl ca-certificates zlib1g-dev \
  qemu-user binutils-arm-linux-gnueabi
```

Для сборки Windows-конвертера дополнительно:

```bash
sudo apt-get install -y gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
```

Предпочтительно хранить рабочие деревья и сборку в Linux-файловой системе `~/`, чтобы сохранить симлинки и избежать медленного обхода NTFS. Репозиторий можно держать на `/mnt/d`; распакованный sysroot и SDK должны оставаться в Linux-файловой системе.

## 2. Получить единственный репозиторий

```bash
mkdir -p ~/src
cd ~/src
git clone https://github.com/alex-13021991/Anbernic-RG35XX.git
cd Anbernic-RG35XX/Jazz2_RG35XX
```

Для Download ZIP: распакуйте архив, перейдите в его `Jazz2_RG35XX`. История Git для сборки не нужна, но программа `git` нужна для применения патчей. Никаких `git submodule update` и клонирования других проектов нет.

## 3. Тулчейн

Используется `arm-miyoo-linux-uclibcgnueabi-g++`, **GCC 7.3.0 / Buildroot 2018.02.9**, с uClibc. Это не Miyoo Mini GCC 8.3 hard-float и не обычный Ubuntu `arm-linux-gnueabihf-g++`.

Архив: [env_toolchain.7z](https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z).

SHA-256:

```text
70d4ea2d477f931eedbc7e049cddd672098a92c7b9e5672bd90b65143db73c70
```

Один раз выберите новый рабочий каталог и установите SDK:

```bash
export JAZZ2_WORK="$HOME/.cache/jazz2-rg35xx-release"
export JOBS=4
bash scripts/setup-toolchain.sh
```

Сценарий скачивает только SDK, проверяет SHA-256 и распаковывает его в `$JAZZ2_WORK/miyoo`. Входящий в SDK sysroot не используется: целевые библиотеки берутся из включённого снимка GarlicOS.

Если архив уже скачан:

```bash
export JAZZ2_TOOLCHAIN_ARCHIVE="/mnt/d/Downloads/env_toolchain.7z"
bash scripts/setup-toolchain.sh
```

Если SDK уже распакован:

```bash
export RG_TOOLCHAIN="$HOME/sdk/miyoo"
export LD_LIBRARY_PATH="$RG_TOOLCHAIN/lib"
"$RG_TOOLCHAIN/bin/arm-miyoo-linux-uclibcgnueabi-g++" --version
unset LD_LIBRARY_PATH
```

Не заменяйте `HOME` и не добавляйте библиотеки ARM в системный `LD_LIBRARY_PATH`. Скрипты сами временно устанавливают путь к **хостовым** библиотекам SDK там, где это нужно.

Оставьте системные `/usr/bin/cmake`, Ninja и pkg-config впереди SDK в `PATH`: в SDK есть старые хостовые утилиты. Наши скрипты добавляют `miyoo/bin` в конец `PATH`, а целевые компиляторы выбирают по абсолютному пути. Для MinGW скрипт конвертера явно задаёт Windows API 0x0602, необходимый старым заголовкам Ubuntu 20.04.

## 4. Полная сборка движка и libopenmpt

В той же оболочке, с сохранёнными `JAZZ2_WORK` и при необходимости `RG_TOOLCHAIN`:

```bash
bash scripts/prepare.sh
bash scripts/build.sh
```

`prepare.sh` проверяет архивы по `vendor/SHA256SUMS`, распаковывает минимальный sysroot и собирает **libopenmpt 0.5.34** из включённых исходников. Обязательная zlib включена: без неё сжатая музыка `.j2b` не воспроизводится. Все необязательные внешние декодеры отключены.

`build.sh` при первом запуске вызывает `source.sh`. Он распаковывает неизменённый `jazz2-native`, применяет по порядку `patches/miyoo/*.patch`, `patches/rg35xx/*.patch` и копирует `platform/*.h`. Затем CMake/Ninja собирает `jazz2`.

Профиль: `-marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp -O2`, SDL2, software RHI / RGB565, без сетевой игры, без OpenGL, без скачивания CMake-зависимостей и без LTO. Загрузчик — `/lib/ld-uClibc.so.0`.

Результаты:

```text
$JAZZ2_WORK/engine/                 итоговое дерево исходников
$JAZZ2_WORK/sysroot/                целевые заголовки и библиотеки
$JAZZ2_WORK/build/jazz2             ARM-бинарник
$JAZZ2_WORK/logs/                   журналы подготовки и компиляции
```

Для повторной сборки изменённого дерева достаточно `bash scripts/build.sh`. Скрипт не перезаписывает существующее `engine` патчами. После изменения серии патчей используйте **новый** `JAZZ2_WORK`; старый кеш можно сохранить для сравнения. Для изменения порта редактируйте патчи либо итоговое дерево осознанно, а не рассчитывайте на автоматическое копирование соседних каталогов.

## 5. Собрать конвертер из тех же исходников

```bash
bash scripts/build-converter.sh linux
bash scripts/build-converter.sh windows
```

Результаты:

```text
$JAZZ2_WORK/converter-linux/packer/AssetPacker
$JAZZ2_WORK/converter-windows/packer/AssetPacker.exe
```

Linux-версия собирается хостовым GCC, Windows-версия — MinGW-w64 x86-64 **POSIX**. Обе используют исходники `AssetPacker` из нашего итогового движка и включённую zlib 1.2.11. Windows EXE статически включает C++ runtime и zlib; дополнительных DLL из других репозиториев не требуется. Сам ARM-тулчейн для этих двух целей не используется. Версия языка конвертера — C++20; необходимые совместимости GCC определяются исходниками.

Готовый Windows EXE уже находится в `Tools`. Для использования собственных сборок скопируйте EXE туда или передайте путь через параметр `--packer` Linux-обёртке. [Конвертация](CONVERTING.md).

## 6. Проверки

Полная сборка по этому сценарию проверена в отдельном окружении Ubuntu 20.04 внутри WSL2, включая libopenmpt и оба конвертера. Подробности, версии и ограничения проверки — [RELEASE-VALIDATION.md](RELEASE-VALIDATION.md).

```bash
bash scripts/verify.sh
```

Без игровых данных проверяются ABI, соответствие Cortex-A9, исправление `memset`, цвета/геометрия framebuffer и модель управления. Включены режимы 16/32 бит и страницы со смещениями.

Если у вас есть собственная преобразованная `Content`:

```bash
bash scripts/verify.sh /путь/к/Content
```

Дополнительно проверяются музыка через OpenAL null, старт игры, уровень `prince/01_castle1`, сохранение настроек, выход MENU и отказ на неподдерживаемом framebuffer. QEMU использует собственные временные сохранения. Физический звук и реальная скорость проверяются на приставке.

## 7. Упаковать релиз без данных игры

```bash
python3 scripts/release.py "$JAZZ2_WORK" "$HOME/jazz2-release"
```

Появятся три раздельных архива:

- `Jazz2_RG35XX_v1.0.0.zip` — ARM-игра с библиотеками и превью для карты;
- `Jazz2_RG35XX_Tools_v1.0.0.zip` — Windows/Linux-конвертеры, свободные `EngineContent` и инструкции для ПК;
- `Jazz2_RG35XX_v1.0.0-source.tar.gz` — весь проект, включая общее дерево исходников и исходные архивы.

Скрипт не берёт вашу `Content`, `Saves` или `Logs`. Он проверяет отсутствие форматов оригинальной игры и пишет контрольные суммы. При существующем выходном каталоге скрипт не перезаписывает его. Готовые архивы доступны в [релизе 1.0.0](https://github.com/alex-13021991/Anbernic-RG35XX/releases/tag/jazz2-rg35xx-v1.0.0).

Оригинальный комплект исходников можно получить обычным Download ZIP GitHub или файлом `Jazz2_RG35XX_v1.0.0-source.tar.gz` из релиза. Архив содержит весь каталог проекта, включая `vendor`, и не требует других репозиториев. Тулчейн и пакеты Ubuntu устанавливаются отдельно, как описано выше.

## Что сохранять перед удалением локальных файлов

Исходники и проектные зависимости восстанавливаются из опубликованного репозитория/архива. Ваши **оригинальная GOG-игра, преобразованная Content и сохранения** в публичный комплект не входят. При необходимости сохраните их отдельно; сборка исходников не восстановит личный игровой прогресс.
