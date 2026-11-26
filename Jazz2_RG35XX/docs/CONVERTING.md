# Подготовка игровых ресурсов GOG

## Проверенная версия

Проверена установленная английская **GOG Jazz Jackrabbit 2 / Jazz Jackrabbit 2 Collection**, содержащая **The Secret Files v1.24**. В проверенном комплекте `ReadMe.txt` указывает Secret Files v1.24, а конвертер сообщает `Detected Jazz Jackrabbit 2: The Secret Files`.

Для точной идентификации исходной установки:

| Файл / поле | Значение проверенной копии |
|---|---|
| GOG gameId | `1351891846` |
| GOG buildId | `50747586883966799` |
| `Anims.j2a` | 9 609 805 байт |
| SHA-256 `Anims.j2a` | `22f80f5db9e89af3c2f9a85d9649c1d0a6d2d2b51b3448aea29366d89df98522` |
| SHA-256 `Jazz2.exe` | `152108e4adafd0e75d0cd1b8784e2e8d8598d954ed95d572e374f755ed193d91` |

Хеши служат для сравнения с проверенной установкой. Отличие GOG-патча не обязательно означает несовместимость, но другие издания, модификации JJ2+ и пользовательские уровни этим аппаратным тестом не проверены. Поле Windows FileVersion у старого `Jazz2.exe` не следует использовать вместо версии данных: в нашем файле оно равно 1,0,0,1.

Нужна **установленная/распакованная игра**, а не файл `setup_*.exe`. Укажите папку, где непосредственно лежат `Anims.j2a`, `Jazz2.exe`, `.j2l`, `.j2t`, музыка и ролики. Если в коллекции несколько подпапок, выберите каталог Secret Files. Не выбирайте папку Jazz Jackrabbit 1 / Jazz Jackrabbit CD.

## Windows: готовый конвертер

1. Распакуйте отдельный архив **Jazz2_RG35XX_Tools_v1.0.0.zip** в обычную папку на ПК. В `Tools` должны находиться `AssetPacker.exe`, `Convert.ps1`, BAT-файл и `EngineContent`. Эти утилиты не входят в установочный архив игры и на карту приставки не копируются.
2. Перетащите папку установленной GOG-игры на `Tools/Convert Jazz Jackrabbit 2.bat` либо откройте BAT и введите путь.
3. Дождитесь окончания. Результат появится в `Tools/Content`.
4. Скопируйте **всю** полученную `Content` в `Roms/PORTS/Jazz Jackrabbit 2/` на карте.

Если Windows блокирует запуск скрипта, используйте PowerShell с разрешением только для этого вызова:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\Tools\Convert.ps1" `
  -Game "C:\GOG Games\Jazz Jackrabbit 2 Collection" `
  -Output "D:\Converted\Jazz2\Content"
```

Указанный выходной каталог **не должен существовать**: так исключается смешивание старой и новой конвертации. Для повторного преобразования задайте другую папку. Входная GOG-установка не изменяется; временная копия создаётся в `%TEMP%`, поэтому нужен запас места для установки и результата.

В результате проверьте наличие:

```text
Content/
  Prebaked.pak
  Prebaked.version
  Episodes/
  Music/
  Translations/ru.mo
  ...
```

Список дополнительных каталогов зависит от содержимого игры. Ключевы два `Prebaked.*` и сохранение всей структуры результата.

## Ubuntu / WSL

При использовании клона репозитория:

```bash
export JAZZ2_WORK="$HOME/.cache/jazz2-rg35xx-release"
bash scripts/build-converter.sh linux
python3 scripts/convert.py \
  "/mnt/c/GOG Games/Jazz Jackrabbit 2 Collection" \
  "$HOME/jazz2-private/Content" \
  --packer "$JAZZ2_WORK/converter-linux/packer/AssetPacker"
```

В `Tools/AssetPacker-linux` также поставляется готовая Linux-версия конвертера, если вы не хотите её собирать. Она предназначена для x86-64 Linux с glibc не старее 2.31. Для установленной копии релиза с обёрткой `convert.py` можно запускать её из `Tools`:

```bash
python3 Tools/convert.py "/mnt/c/GOG Games/Jazz Jackrabbit 2 Collection" \
  "$HOME/jazz2-private/Content" --packer Tools/AssetPacker-linux \
  --engine-content Tools/EngineContent
```

## Что именно делает преобразование

Обёртки готовят соседние `Source` и `Content`, копируя в `Content` только свободные исходные ресурсы движка из `EngineContent`. Затем вызывают:

```text
AssetPacker STAGING OUTPUT --target=console --originals-only --skip-non-episode-levels --video-downscale=2
```

`--target=console` создаёт готовые `Prebaked.*`, чтобы приставке не приходилось конвертировать данные. Два фильтра оставляют штатные эпизоды вместо сетевых и посторонних уровней. Видеоролики уменьшаются вдвое: 640×480 → 320×240. После конвертации обёртка дополнительно копирует имеющиеся `menu.j2b`, `bonus2.j2b`, `bonus3.j2b`, `intro.j2b`, `ending.j2b` в `Music`: эти дорожки могут не ссылаться напрямую из уровней.

В проверенной GOG-копии конвертер пропустил 51 уровень по заданным фильтрам; это ожидаемо. Предупреждения об атласе 1024×1024 для некоторых анимаций/тайлсетов также встречались у успешно работающего комплекта. Ориентируйтесь на итоговый код завершения, сообщение `Done` и присутствие выходных файлов; ошибки чтения исходных данных не игнорируйте.

На приставке журнал должен сообщить `Content ... was prepared ahead of time`, успешное монтирование `Prebaked.pak` и загрузку `ru.mo`. Не помещайте `Anims.j2a` и исходные уровни прямо в папку приложения вместо результата конвертации.

Конвертированная `Content`, GOG-оригиналы и личные сохранения не публикуются. При переустановке можно использовать уже подготовленную рабочую `Content`; пересоздавать её ради этого релиза не требуется.
