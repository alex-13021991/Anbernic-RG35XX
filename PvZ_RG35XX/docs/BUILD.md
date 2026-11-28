# Воспроизводимая локальная сборка
Ubuntu 24.04 x86_64 / WSL Ubuntu-24.04. Храните клон и рабочую папку в Linux ext4, не в /mnt/c или /mnt/d: SDK использует Unix-права и ссылки. Желательно 5 ГБ свободного места.

Системные инструменты (это единственная установка из сети, если пакеты ещё не установлены):
```
sudo apt-get install build-essential cmake ninja-build pkg-config python3 p7zip-full git binutils patch qemu-user
```
Тулчейн в игровой репозиторий и исходный архив НЕ включён. Возьмите Miyoo GCC 7.3.0 из собственного [RG35XX-Development](https://github.com/alex-13021991/RG35XX-Development). Используйте его штатную инструкцию объединения и распаковки частей. По умолчанию ожидается:
`~/RG35XX/RG35XX-SDK/miyoo-gcc-7.3.0/bin/arm-linux-g++`.
Другой путь: `export RG_TOOLCHAIN=/absolute/path/to/extracted/miyoo`.

После получения одного собственного репозитория игры и установки SDK:
```
bash scripts/build.sh
bash scripts/test.sh
python3 scripts/package.py --out "$PWD/dist"
```
Ни build.sh, ни prepare.sh не клонируют репозитории и не скачивают зависимости. CMake отключает загрузку аудиокодеков и FetchContent. Все исходники игры, libopenmpt и системный sysroot включены в проект; внешний SDK используется через RG_TOOLCHAIN. Системные пакеты Ubuntu в комплект не входят.

GCC 7.3.0 Cortex-A9 NEON softfp, C++17, uClibc, Release. Не заменяйте на arm-linux-gnueabihf. prepare.sh проверяет версию существующего компилятора и суммы vendor, распаковывает sysroot, собирает libopenmpt с mpg123/ogg/vorbis/vorbisfile и затем PvZ/конвертер/тесты. Логи: .work/logs/. Ресурсы игры для компиляции не нужны.

Для другого рабочего каталога: `PVZ_WORK=/absolute/path bash scripts/build.sh`. Эту же переменную задавайте тестам и упаковщику. JOBS задаёт число компиляций. Для чистой пересборки выберите новый пустой PVZ_WORK. Не удаляйте исходники, оригинальные клоны и SDK пользователя.

scripts/verify-patches.py распаковывает вложенный upstream, применяет patches/series и сравнивает всё содержимое с sources/pvz. Инфраструктура релиза находится непосредственно в корне проекта. Патчи при обычной сборке накладывать не нужно: полный итоговый код уже включён.
