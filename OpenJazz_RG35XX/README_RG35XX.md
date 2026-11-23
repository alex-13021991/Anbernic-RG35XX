# OpenJazz RG35XX — GarlicOS 1.4.9

Модифицированный OpenJazz для оригинального
Anbernic RG35XX (не Plus, не H700).

Основа: [AlisterT/OpenJazz](https://github.com/AlisterT/openjazz),
коммит `f2aed10` (Implement game saving).

## Проверенные игры и обновление до v1.1

Порт проверен на оригинальной Anbernic RG35XX с GarlicOS 1.4.9 и файлами игр из GOG-релиза. Обе игры работают с этими данными. Названия установочных архивов и каталоги на карте приведены ниже.

| Установочный архив | Игра | Файл движка на карте |
|---|---|---|
| `Jazz Jackrabbit CD.ZIP` | Jazz Jackrabbit CD | `PORTS/Jazz Jackrabbit/OpenJazz` |
| `Jazz Holiday 95.ZIP` | Jazz Jackrabbit Holiday Hare '95 | `PORTS/Jazz Holiday 95/OpenJazz` |

В v1.1 убрана вторая привязка игрового меню к L1. После обновления меню открывается по SELECT, пауза включается по START. Обработка кнопок проверена также в ARM-эмуляторе с конфигурациями обеих игр, включая сохранение назначений и повторный запуск.

Для обновления установленной игры закройте её и распакуйте `OpenJazz_RG35XX_v1.1.zip` в каталог карты, содержащий папку `PORTS`. При обновлении одной игры скопируйте только её файл движка из таблицы. Сохранения и настройки остаются на карте. Для запуска нужны игровые данные из собственной копии игры.

## Возможности

| Кнопка | Действие |
|---|---|
| MENU | Выход в GarlicOS |
| SELECT | Игровое меню |
| L1 | Свободна для назначения в настройках управления |
| START | Пауза |
| VOL+ / VOL- | Изменение громкости |

Дополнительно:
- Сохранение и загрузка игры
- Сохранение настроек при выходе
- Скрытый курсор мыши
- SDL 1.2, ARM/uClibc

## Сборка

Требуется MiyooCFW ARM/uClibc SDK.

```bash
cmake -S . -B build-rg35xx -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/opt/miyoo/share/buildroot/toolchainfile.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DLEGACY_SDL=ON -DRG35XX=ON \
  -DPORTABLE=ON \
  -DNETWORK=OFF \
  -DSCALE=OFF \
  -DENABLE_JJ2=OFF

cmake --build build-rg35xx -j 8
```

Исходники с внесёнными изменениями и ARM-бинарник
доступны в разделе Releases.

## Авторы и лицензия

Оригинальный движок: AJ Thomson, Carsten Teibes
и другие участники OpenJazz.

Порт для RG35XX основан на оригинальном OpenJazz
и доработках для GarlicOS.

Лицензия движка: GPL-2.0-or-later.

Файлы оригинальной игры Jazz Jackrabbit не включены.
