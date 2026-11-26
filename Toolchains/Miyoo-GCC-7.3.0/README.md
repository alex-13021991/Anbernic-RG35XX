# Miyoo GCC 7.3.0 — архив и настройка

## Что это

Готовый SDK для **Linux x86-64**, содержащий кросс-компиляторы C/C++ для ARM с uClibc:

- GCC **7.3.0**, Buildroot **2018.02.9**;
- префикс: `arm-miyoo-linux-uclibcgnueabi-`;
- uClibc **1.0.28** в собственном SDK;
- программы компилятора запускаются в Linux / WSL, а результат предназначен для ARM-устройства;
- проверен для порта Jazz² Resurrection на **оригинальную RG35XX (2022), GarlicOS 1.4.9**.

Это не Windows-компилятор, не тулчейн Miyoo Mini Cortex-A7 hard-float и не SDK для RG35XX Plus / H700. Параметры исходной конфигурации GCC по умолчанию не являются профилем RG35XX: нужные CPU/ABI задаются явно при сборке приложения.

## Происхождение и сохранность

Исходный файл — `env_toolchain.7z` из [релиза steward-fu/website](https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z). Это архив готовых инструментов стороннего поставщика, а не собранный нами GCC. Его содержимое не перепаковывалось и не изменялось; для хранения в Git сделано только последовательное разделение байтов на части.

Полный размер: **128 828 510 байт** — 122,86 МиБ / 128,83 МБ. Существующая распакованная рабочая копия занимала около **905 МиБ**; фактический объём зависит от распаковки, исключённых каталогов и файловой системы. Отдельно требуется место под проект, sysroot, временные файлы и сборку.

SHA-256 полного архива:

```text
70d4ea2d477f931eedbc7e049cddd672098a92c7b9e5672bd90b65143db73c70
```

В этой папке находятся:

| Файл | Размер |
|---|---:|
| `env_toolchain.7z.001` | 50 331 648 байт |
| `env_toolchain.7z.002` | 50 331 648 байт |
| `env_toolchain.7z.003` | 28 165 214 байт |
| `SHA256SUMS` | Контрольные суммы частей |
| `ARCHIVE-SHA256.txt` | Контрольная сумма объединённого архива |
| `manifest.json` | Версия, источник, размеры и хеши |
| `assemble.py` | Объединение и автоматическая проверка |

Части меньше лимита обычного Git-файла и включены непосредственно в репозиторий. Нужны **все три** части из одной папки. Переименовывать их не нужно. Распаковывать каждую часть отдельно не следует.

## WSL Ubuntu 20.04: установка

В PowerShell Windows при отсутствии WSL:

```powershell
wsl --install -d Ubuntu-20.04
wsl -d Ubuntu-20.04
```

Далее команды выполняются в **Bash Ubuntu**:

```bash
sudo apt-get update
sudo apt-get install -y python3 p7zip-full build-essential cmake ninja-build \
  git pkg-config ca-certificates
cd ~/src/Anbernic-RG35XX/Toolchains/Miyoo-GCC-7.3.0
python3 assemble.py "$HOME/Downloads/env_toolchain.7z"
```

Скрипт проверяет размер и SHA-256 каждой части и итогового файла. Существующий выходной файл не перезаписывается. Если места недостаточно или часть повреждена, операция завершится ошибкой. Успешный результат содержит исходный SHA-256, приведённый выше.

Объединить можно и стандартными средствами Linux:

```bash
sha256sum -c SHA256SUMS
mkdir -p "$HOME/Downloads"
cat env_toolchain.7z.001 env_toolchain.7z.002 env_toolchain.7z.003 \
  > "$HOME/Downloads/env_toolchain.7z"
echo '70d4ea2d477f931eedbc7e049cddd672098a92c7b9e5672bd90b65143db73c70  '"$HOME/Downloads/env_toolchain.7z" | sha256sum -c -
7z t "$HOME/Downloads/env_toolchain.7z"
```

Команда `cat` перезаписывает выбранный файл; используйте новый путь. В отличие от неё `assemble.py` отказывается заменять уже существующий файл.

### Полная распаковка и проверка

Храните SDK в Linux-файловой системе WSL, например в `~/sdk`, чтобы сохранить права исполнения и ссылки:

```bash
mkdir -p "$HOME/sdk"
7z x "$HOME/Downloads/env_toolchain.7z" -o"$HOME/sdk"
export RG_TOOLCHAIN="$HOME/sdk/miyoo"
export PATH="$PATH:$RG_TOOLCHAIN/bin"
export LD_LIBRARY_PATH="$RG_TOOLCHAIN/lib"
"$RG_TOOLCHAIN/bin/arm-miyoo-linux-uclibcgnueabi-g++" --version
"$RG_TOOLCHAIN/bin/arm-miyoo-linux-uclibcgnueabi-g++" -dumpmachine
unset LD_LIBRARY_PATH
```

Ожидаются GCC 7.3.0 и `arm-miyoo-linux-uclibcgnueabi`. Не изменяйте `HOME`, не заменяйте системные компиляторы и не копируйте библиотеки SDK в `/usr/lib`.

В SDK присутствуют старые **хостовые** утилиты, включая CMake. Поэтому `miyoo/bin` добавляется **в конец PATH**, а целевой компилятор выбирается абсолютным путём. Для запуска GCC могут понадобиться хостовые библиотеки из `miyoo/lib`; ограничивайте `LD_LIBRARY_PATH` конкретной командой/сценарием, не делайте его глобальной настройкой всей WSL.

## Использование с Jazz² Resurrection

Из полного клона или Download ZIP репозитория:

```bash
cd ~/src/Anbernic-RG35XX/Jazz2_RG35XX
export JAZZ2_WORK="$HOME/.cache/jazz2-rg35xx-release"
export JOBS=4
bash scripts/setup-toolchain.sh
bash scripts/prepare.sh
bash scripts/build.sh
```

`setup-toolchain.sh` обнаруживает соседнюю папку `Toolchains/Miyoo-GCC-7.3.0`, собирает и проверяет локальный архив, затем устанавливает SDK. Интернет для получения SDK в этом случае не нужен. Уже установленный компилятор повторно не распаковывается. Для отдельного исходного архива Jazz2 1.0.0, выпущенного до добавления этой папки, остаётся прежний вариант установки SDK из файла или исходной ссылки.

Если SDK установлен вручную, передайте `RG_TOOLCHAIN`. Рабочий профиль Jazz2:

```text
-marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp
```

Используется **целевой sysroot GarlicOS** из `Jazz2_RG35XX/vendor/garlicos-1.4.9-build-sysroot.tar.gz`, а не libc из SDK. Они имеют одинаковую строку версии, но разные байты. Подмена системных библиотек приставки не требуется.

[Подробная инструкция сборки Jazz2](../../Jazz2_RG35XX/docs/BUILD.md) · [Происхождение библиотек](../../Jazz2_RG35XX/docs/DEPENDENCIES.md)

## Windows без распаковки SDK

Для обычной игры компилятор не нужен. Скачать файлы можно через Download ZIP репозитория. При установленном 7-Zip открывайте `env_toolchain.7z.001`, оставив остальные части рядом. Для дальнейшей сборки используйте Linux/WSL: запуск ARM-кросс-компилятора из этого SDK непосредственно в Windows не предусмотрен.

## Лицензии

Компоненты архива сохраняют исходные лицензии. GCC/binutils относятся к семейству GNU GPL; библиотеки runtime могут иметь исключения и отдельные лицензии. Наличие GCC Runtime Library Exception не меняет лицензию самого компилятора. Уведомления и файлы лицензий внутри исходного SDK сохраняются без изменений. Этот архив не содержит оригинальные данные GOG-игры или ваши сохранения.
