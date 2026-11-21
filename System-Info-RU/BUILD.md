# Сборка System Info RU 1.0.0

Цель — оригинальная Anbernic RG35XX, GarlicOS 1.4.9, ARMv7 Cortex-A9, 32-битный EABI5, вызовы soft-float, загрузчик `/lib/ld-uClibc.so.0`. Приложение динамически использует SDL 1.2, SDL_ttf и uClibc. Для ввода в комплекте тот же адаптер j2k.so, что в USB Manager 0.1.4.

## Сборка и упаковка

Нужны Python 3.8+, Zig 0.13.0 и оригинальный Miyoo SDK с uClibc и GCC 7.3.0. Параметр `--sdk` указывает на каталог, содержащий `arm-miyoo-linux-uclibcgnueabi/sysroot`. При распаковке SDK сохраните цели символических ссылок. Заголовки SDL и стартовые объекты берутся из SDK. Для компиляции используется вход Clang `zig -cc1`, для линковки — `zig ld.lld`.

```sh
python build.py --sdk /opt/miyoo --zig /opt/zig/zig
python verify.py --sdk /opt/miyoo
python package.py --out dist
```

В Windows передайте пути к каталогу SDK и к zig.exe. Команды выполняются из корня распакованных исходников. Готовый ELF появляется в build/system-info. Последняя команда собирает оба ZIP и сверяет каждую запись с исходным файлом.

Инструменты доступны у авторов [Zig](https://ziglang.org/download/0.13.0/) и [архива Miyoo SDK](https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z).

## Проверка подсчёта и управления

На Linux тесту нужны компилятор C и временный каталог на файловой системе с поддержкой разреженных файлов. Два таких файла имитируют большой дисковый образ. Пример команд из корня исходников.

```sh
mkdir -p build
python3 tests/fixtures.py build/fixture
cc -std=gnu99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Isrc tests/test_storage.c src/storage.c src/cache.c src/diag.c -o build/test-storage
(cd build/fixture && ../test-storage)
cc -std=gnu99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Isrc -include tests/ui_fixture.h src/main.c src/screens.c src/storage.c src/cache.c src/diag.c tests/ui_fixture.c -o build/test-ui
(cd build/fixture && ../test-ui)
```

Создавайте тестовую коллекцию в пустом каталоге. Навигационный тест подставляет события кнопок, показания приборов и отложенный запуск сканирования. Он выполняет основной цикл приложения, код экранов и сканер и выводит команды рисования. Для повторения на WASI предусмотрены tests/run-wasi.cjs (чтение каталога на Windows) и tests/wasi_temp.* (создание временного файла кэша).

## Проверки сборки

Исполняемый файл собран для ARM. Проверены класс ELF, машинная архитектура, EABI, интерпретатор, зависимости и разрешение импортов в библиотеках SDK. На Windows в WASI исполнен тот же код подсчёта в быстром и полном режимах. Проверены отдельные TF1/TF2, M3U/CUE/BIN, CCD/IMG, итог больше 4 ГиБ, APPS, сейвы двух профилей, состояния, скриншоты, BIOS, отмена, пустая карта, повреждённый кэш и замена карты. Для интерфейса проверены смена вкладок, выбор карт, переход к платформам, отмена, диагностические записи и размещение текста при 640×480 на тестовых данных. Физическая приставка и её датчики в проверках не участвовали.

