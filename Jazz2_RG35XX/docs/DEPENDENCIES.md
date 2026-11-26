# Исходники, бинарные зависимости и происхождение

Все перечисленные входные данные хранятся в `Jazz2_RG35XX`, кроме самого тулчейна и системных пакетов Ubuntu. Ссылки ниже фиксируют происхождение, а не являются обязательными шагами скачивания для сборки проекта. SHA-256 архивов записаны в `vendor/SHA256SUMS`; полный пофайловый манифест sysroot — `vendor/sysroot-manifest.json`.

## Исходные проекты

| Компонент | Зафиксированное состояние | Локальная копия | Исходный источник |
|---|---|---|---|
| Jazz² Resurrection | 3.7.0, `b8cba705fb19cb8c747f8367814db47374094f81` | `vendor/jazz2-native-b8cba705.tar.gz` | [deathkiller/jazz2-native](https://github.com/deathkiller/jazz2-native/tree/b8cba705fb19cb8c747f8367814db47374094f81) |
| Miyoo-порт | `3de963ee96dc8db505a6eae4f504659dbb24b16e` | `vendor/jazz2-miyoo-3de963ee.tar.gz` | [ArticlessCZ-Coder/jazz2-miyoo](https://github.com/ArticlessCZ-Coder/jazz2-miyoo/tree/3de963ee96dc8db505a6eae4f504659dbb24b16e) |
| libopenmpt | 0.5.34, release.autotools | `vendor/libopenmpt-0.5.34.tar.gz` | [официальный архив](https://lib.openmpt.org/files/libopenmpt/src/libopenmpt-0.5.34+release.autotools.tar.gz) |
| OpenAL Soft | 1.18.1 | `vendor/openal-soft-1.18.1.tar.gz` | [официальный тег](https://github.com/kcat/openal-soft/tree/openal-soft-1.18.1) |
| zlib | 1.2.11 | `vendor/zlib-1.2.11.tar.gz` | [официальный архив](https://zlib.net/fossils/zlib-1.2.11.tar.gz) |

Исходные архивы `jazz2-*` содержат полные деревья указанных commits, без `.git` и без локальных модификаций. Они не требуют обращения к истории Git. Эти же полные деревья уже доступны обычными файлами в `sources/jazz2-native` и `sources/jazz2-miyoo`, исходники библиотек — в `sources/libraries`. Канонические архивы сохранены дополнительно для точного восстановления базы и симлинков независимо от настроек перевода строк Git. `source.sh` собирает итоговое дерево из них, затем применяет все патчи.

При желании распаковать эталонные архивы отдельно:

```bash
mkdir -p ~/jazz2-reference/jazz2-native ~/jazz2-reference/jazz2-miyoo
tar xf vendor/jazz2-native-b8cba705.tar.gz -C ~/jazz2-reference/jazz2-native
tar xf vendor/jazz2-miyoo-3de963ee.tar.gz -C ~/jazz2-reference/jazz2-miyoo
```

`patches/miyoo/0001-miyoo-port.patch` соответствует патчу из сохранённого Miyoo-репозитория. `patches/rg35xx/0001-garlic.patch` и `0002-runtime-fixes.patch`, а также `platform/*.h` — адаптация для RG35XX. Порядок применения задан в `scripts/source.sh`. Различия платформ существенны: инструкции и готовые библиотеки Miyoo Mini Cortex-A7 hard-float из исходного порта не используются на RG35XX Cortex-A9 softfp.

## Что действительно собиралось

- **Движок** — из исходного дерева с обеими сериями патчей, Miyoo GCC 7.3.0.
- **libopenmpt 0.5.34** — из включённого autotools-архива тем же ARM-компилятором, с zlib, без необязательных mpg123/ogg/vorbis/PortAudio/PulseAudio/sndfile/FLAC. Полный рецепт — `scripts/prepare.sh`.
- **AssetPacker** — из тех же исходников движка. Для Windows используется MinGW-w64 x86-64 POSIX, для Linux — хостовый GCC.
- **zlib 1.2.11 для конвертера** — статически из включённых исходников. Это отдельная хостовая сборка; ARM-версия zlib в игре взята готовой.

OpenAL Soft в аппаратно проверенном комплекте **не пересобиралась**: использована готовая библиотека из sysroot. Архив OpenAL первоначально использовался для заголовков AL. Его полные исходники сохранены дополнительно; они не заменяют автоматически библиотеку проверенного снимка и не доказывают совпадение всех настроек её первоначальной сборки.

## Готовые ARM-библиотеки и sysroot

Основа — локальный снимок **GarlicOS 1.4.9 с оригинальной RG35XX**, сбор `20261125T155126Z`. Исходный архив sysroot до отбора:

```text
SHA256 570e643da495f3131af254051a73a8d5225ea3e1811288a4e27acfdee77f215e
```

Для автономного проекта сохранён необходимый поднабор, а не полный образ прошивки:

| Файлы / версия | Откуда взяты | Как используются |
|---|---|---|
| SDL2 `libSDL2-2.0.so.0.8.0` | Библиотеки приставки из снимка GarlicOS | Связь с SDL и событиями, окно dummy; вывод в fb0 реализован нашим кодом |
| OpenAL `libopenal.so.1.18.1` | Тот же снимок | Звук, ALSA backend |
| zlib `libz.so.1.2.11` | Тот же снимок | Сжатие ресурсов и J2B |
| tslib `libts.so.0.9.0` | Тот же снимок | Транзитивная зависимость SDL |
| libstdc++ `6.0.24`, libgcc_s, libatomic | Тот же снимок; совместимость с GCC 7.3 | C++ / ARM runtime |
| uClibc 1.0.28 и загрузчик | Именно библиотеки GarlicOS, не подмена из SDK | Линковка и QEMU; libc остаётся системной на устройстве |
| libasound и нужные ссылки | Тот же снимок | ALSA API из окружения GarlicOS |
| Заголовки C/Linux/SDL/ALSA/C++ и CRT, ld script, nonshared archives | Miyoo SDK 7.3.0; AL — архив OpenAL 1.18.1 | Компиляция / линковка |

Версия в полном имени бинарника приводится как идентификатор поставленного файла. Для конкретной сборки и возможных vendor-изменений авторитетны байты и хеши, а не предположение о совпадении с чистым upstream-релизом. Эти готовые библиотеки в данном проекте не пересобираются; их исходники не выдаются за восстановленные исходники vendor-сборок.

Проверенные хеши:

```text
libuClibc-1.0.28.so (GarlicOS)
efeae572ed5300c77ebe11f818c8aa4e3748cc1d3266b82bba06d9167aa2d7b9
libopenal.so.1.18.1
dff5efaf5c24ea067a1e653a6362dbcee2daf65db586cee1f288779ab936466a
libopenmpt.so.0.2.2 (наша исходная аппаратно проверенная сборка)
b1634e9e68b4e01c4f2b689f3465d01dd432e75ba54bc929c118741b45d8e5ee
```

В архиве sysroot отсутствует готовая libopenmpt: `prepare.sh` обязательно собирает её из исходников на чистом окружении. Неиспользуемые библиотеки прошивки и Boost-заголовки не включены. Симлинки хранятся внутри TAR, поэтому Download ZIP на Windows не повреждает связи sysroot.

В релизном приложении локально лежит замыкание динамических зависимостей. Системная `libc.so.0` не заменяется. Из-за `DT_NEEDED` в поставленной libstdc++ также сохраняется alias загрузчика `ld-uClibc.so.1` из того же снимка.

## Лицензии и авторство

- Jazz² Resurrection / Miyoo-порт / наши изменения: GPL-3.0, авторы исходного движка и порта сохранены в архивах.
- libopenmpt: BSD-3-Clause, включая собственные notices вложенных компонентов в архиве.
- OpenAL Soft: LGPL; точный текст находится в `COPYING` исходного архива.
- SDL2 и zlib: zlib license.
- tslib / uClibc: семейство LGPL, уведомления исходных проектов.
- GCC runtime / libstdc++: GPL с GCC Runtime Library Exception; исключение не меняет лицензию самого компилятора.

Тексты и уведомления собраны в `LICENSES`, а исходные notices также остаются в исходных архивах. Дополнительные источники происхождения готовых компонентов: [SDL](https://github.com/libsdl-org/SDL), [tslib](https://github.com/libts/tslib), [uClibc-ng](https://uclibc-ng.org/), [GCC 7.3](https://gcc.gnu.org/releases/gcc-7.3.0/). Для восстановления этого порта скачивать их не требуется: используются сохранённые готовые ARM-файлы.

Готовое превью — созданная с помощью imagegen фан-иллюстрация, с шаблоном размещения владельца проекта; исходник и prompt сохранены в `assets`. Ресурсы `EngineContent` взяты из неизменённого дерева движка. Оригинальные GOG-данные, преобразованная `Content` и сохранения в публичный комплект не включены.
