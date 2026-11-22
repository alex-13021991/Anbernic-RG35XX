# OpenJazz RG35XX — GarlicOS 1.4.9

Модифицированный OpenJazz для оригинального
Anbernic RG35XX (не Plus, не H700).

Основа: [AlisterT/OpenJazz](https://github.com/AlisterT/openjazz),
коммит `f2aed10` (Implement game saving).

## Возможности

| Кнопка | Действие |
|---|---|
| MENU | Выход в GarlicOS |
| SELECT | Игровое меню |
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
  -DLEGACY_SDL=ON \
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
