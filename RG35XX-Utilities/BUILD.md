# Сборка Screenshot Viewer и USB Manager 0.1.5

Для сборки в WSL 2 с Ubuntu нужен оригинальный [Miyoo SDK](https://github.com/steward-fu/website/releases/download/miyoo/env_toolchain.7z) и Python 3. Распакуйте SDK на файловую систему Linux с сохранением символических ссылок. SHA-256 архива `env_toolchain.7z` — `70d4ea2d477f931eedbc7e049cddd672098a92c7b9e5672bd90b65143db73c70`.

Сборка использует GCC 7.3.0, Cortex-A9, ARMv7-A, softfp и загрузчик `/lib/ld-uClibc.so.0`. Ключи `-Os` и `-flto` оптимизируют размер, компоновщик удаляет неиспользуемые секции и отладочные символы.

Если вы работаете с клоном GitHub, выполните `python3 RG35XX-Utilities/collect.py build-tree` из корня репозитория и перейдите в `build-tree`. При работе с архивом исходников откройте каталог, где лежит `build-wsl.sh`.

```sh
sh build-wsl.sh /opt/miyoo
python3 package.py --out dist
```

Бинарники появятся в `build/screenshot-viewer` и `build/usb-manager`. Каталог `dist` будет содержать по установочному ZIP на приложение и один общий архив исходников. Проверки USB-настроек запускаются командой `python3 tests/test_usb.py`. Для старого варианта сборки Windows с Zig 0.13 в исходниках сохранён `build.py`.
