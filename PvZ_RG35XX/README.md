# Plants vs. Zombies Portable — RG35XX 1.0.1
Версия 1.0.1 проверена владельцем на реальной приставке; [протоколы и история](docs/TESTING-RU.md).

Порт для **оригинальной RG35XX, GarlicOS 1.4.9, Cortex-A9/softfp/uClibc**. Не сборка для RG35XX Plus/H/SP.

По умолчанию игра рисует в **640×480** и выводит изображение на весь экран 640×480. Логические координаты игры остаются 800×600. Настройка `Roms/PORTS/PlantsVsZombies/render-width.txt`: **320, 400, 480, 640, 800**. Высота соответственно 240, 300, 360, 480, 600. После изменения полностью перезапустите игру. 600 не поддерживается; отсутствующее/неверное значение заменяется на 640.

## Установка
Распакуйте установочный ZIP в корень TF2 с объединением каталога Roms. Запуск: PORTS → PlantsVsZombies. Подробности и управление: [INSTALL-RU.txt](docs/INSTALL-RU.txt).

Публичный архив не содержит ресурсов коммерческой игры. Подготовка из своего PC GOTY `main.pak`:
```
python3 scripts/prepare-assets.py /path/to/main.pak /path/to/card/Roms/PORTS/PlantsVsZombies --properties /path/to/PC-game/properties
```
Личный `ready-to-play.zip` уже содержит подготовленные ресурсы пользователя и предназначен для его установки. Не включайте его в публичный релиз.

## Сборка из одного клона
В репозитории включены полный исходный код порта, исходники libopenmpt, базовый снимок upstream, патч и sysroot. Подмодулей и загрузки зависимостей из других репозиториев нет. Тулчейн вынесен в [RG35XX-Development](https://github.com/alex-13021991/RG35XX-Development); исходники игры из других репозиториев подтягивать не нужно. Нужны установленный Miyoo GCC 7.3.0 и системные инструменты Ubuntu; см. [BUILD.md](docs/BUILD.md).
```
bash scripts/build.sh
bash scripts/test.sh
python3 scripts/package.py --out "$PWD/dist"
```
Результат сборки: `.work/build/PvZ`. SDK не входит в этот комплект; рабочие файлы не меняют исходники. Можно задать абсолютный `PVZ_WORK`.

[История изменений](docs/CHANGELOG-RU.md) · [Результаты тестов](docs/TESTING-RU.md) · [Устройство порта](docs/ARCHITECTURE.md) · [Состав и лицензии](docs/DEPENDENCIES.md) · [Подготовка GitHub-релиза](docs/RELEASE-GITHUB.md).

This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights reserved. (http://popcapframework.sourceforge.net/).

SELECT на уровне: ×0,5 → ×2 → ×1. Музыка сохраняет обычный темп. Удержание MENU ~1,5 с выполняет штатное сохранение уровня/профиля и выход, с теми же ограничениями сохранения, что и меню игры. Превью: Roms/PORTS/Imgs/PlantsVsZombies.png.
