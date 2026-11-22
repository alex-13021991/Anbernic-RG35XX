# Сборка System Info RU 1.1.1

Для сборки в WSL 2 с Ubuntu нужен оригинальный [Miyoo SDK](https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z) и Python 3. Распакуйте SDK на файловую систему Linux с сохранением символических ссылок. SHA-256 архива `env_toolchain.7z` — `70d4ea2d477f931eedbc7e049cddd672098a92c7b9e5672bd90b65143db73c70`.

Сборка использует GCC 7.3.0, Cortex-A9, ARMv7-A, softfp и загрузчик `/lib/ld-uClibc.so.0`. Ключи `-Os` и `-flto` оптимизируют размер, компоновщик удаляет неиспользуемые секции и отладочные символы.

В каталоге `System-Info-RU` или в корне распакованных исходников выполните команды.

```sh
sh build-wsl.sh /opt/miyoo
python3 verify.py --sdk /opt/miyoo
python3 package.py --out dist
```

Готовый бинарник появится в `build/system-info`, установочный архив и исходники — в `dist`. Для прежнего варианта сборки Windows с Zig 0.13 в исходниках сохранён `build.py`.

## Проверка подсчёта и управления

На Linux тесту нужны компилятор C и временный каталог на файловой системе с поддержкой разреженных файлов. Два таких файла имитируют большой дисковый образ. Пример команд из корня исходников.

```sh
mkdir -p build
python3 tests/fixtures.py build/fixture
cc -std=gnu99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Isrc tests/test_storage.c src/storage.c src/cache.c src/catalog.c -o build/test-storage
(cd build/fixture && ../test-storage)
cc -std=gnu99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -Isrc -include tests/ui_fixture.h src/main.c src/screens.c src/storage.c src/cache.c src/catalog.c tests/ui_fixture.c -o build/test-ui
(cd build/fixture && ../test-ui)
(cd build/fixture && SYSINFO_FIXTURE=photos ../test-ui)
(cd build/fixture && SYSINFO_FIXTURE=long-list ../test-ui)
```

Создавайте тестовую коллекцию в пустом каталоге. Навигационный тест подставляет события кнопок, показания приборов и отложенный запуск сканирования. Он выполняет основной цикл приложения, код экранов и сканер и выводит команды рисования. Для повторения на WASI предусмотрены tests/run-wasi.cjs (чтение каталога на Windows) и tests/wasi_temp.* (создание временного файла кэша).

## Проверки сборки

Исполняемый файл собран для ARM. Проверены ELF, EABI, загрузчик и импорты из SDK. Код сканера прошёл проверки для одной и двух карт, M3U/CUE/BIN, CCD/IMG, сумм свыше 4 ГиБ, приложений, сохранений, состояний, скриншотов и BIOS. Проверены отмена, пустая карта, повреждение кэша и замена карты.

В тестовом наборе по фотографиям TF1 и TF2 дают 282 и 955 игр. Общая сводка показывает 1 237 игр, строка FC — 808. Проверены все три порядка сортировки, равенство значений, список из 512 платформ, прокрутка и стрелка над активным столбцом. На макете 640×480 нижняя строка с кнопкой Y занимает около 574 пикселей из 604 при штатном FreeSans 18. Проверки 1.1.0 выполнены на тестовом стенде.

