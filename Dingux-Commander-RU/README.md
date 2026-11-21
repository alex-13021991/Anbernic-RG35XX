# Dingux Commander Dark RU

Двухпанельный файловый менеджер для оригинальной Anbernic RG35XX с GarlicOS 1.4.9. На русский переведены меню, подтверждения файловых операций, сведения о дисках и команды текстового редактора. Раскладка клавиатуры остаётся английской.

**Стабильный выпуск от 21 ноября 2026 года.** Собран в Ubuntu 24.04 под WSL 2 с Miyoo GCC 7.3.0. Владелец подтвердил работу на оригинальной RG35XX с GarlicOS 1.4.9.

[Скачать установочный ZIP](https://github.com/alex-13021991/Anbernic-RG35XX/releases/download/dingux-commander-dark-ru-2026.11.21/DinguxCommanderDark-RU-2026.11.21-GarlicOS-1.4.9.zip) · [Инструкция по установке](Установка.txt)

### Установка

1. Выключите приставку. Сохраните копию своей папки `Roms/APPS/DinguxCommander` и скрипта запуска.
2. Если Dingux Commander уже установлен, замените только файл `Roms/APPS/DinguxCommander/DinguxCommander` одноимённым файлом из архива. При необходимости добавьте `res/FreeSans.ttf`. Так сохранятся ваши настройки.
3. Для полной установки скопируйте содержимое папки `APPS` из архива в `Roms/APPS` на карте. При этом также копируется файл настроек `commander.cfg`.
4. Для обложки меню поместите приложенный PNG в `Roms/APPS/Imgs/Dingux Commander.png`.

![Обложка Dingux Commander](Dingux%20Commander.png)

[Инструкция по сборке](BUILD.txt) · [Патч русификации](russian.patch) · [Лицензия](src/LICENSE.txt)
