#!/bin/sh
# Русский переключатель для IonAgorria/rg35xx-mtp-server, GarlicOS 1.4.9.
# Меняет настройку следующей загрузки, не прерывает текущую передачу файлов.
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
show_message() {
  printf '%s\n' "$2"
  if [ -x "$APP_DIR/Toggle USB/$1" ]; then
    "$APP_DIR/Toggle USB/$1" "   $2   "
  fi
  sleep 3
}
fail() { show_message prntred "$1"; exit 1; }

if [ ! -f /misc/enableMTP ]; then
  [ -f /misc/usb.sh ] && [ -f /mnt/mmc/CFW/autostart/mtp.sh ] &&
    [ -f /mnt/mmc/CFW/mtp/run.sh ] && [ -x /mnt/mmc/CFW/mtp/mtp-server ] ||
    fail "Сначала установите MTP-сервер. См. инструкцию."
  grep -q '^[[:space:]]*/misc/usb\.sh[[:space:]]*$' /misc/dmenu.bin ||
    fail "Добавьте запуск /misc/usb.sh в dmenu.bin."
  action=on
else
  action=off
fi

mount -o remount,rw /misc || fail "Ошибка: раздел MISC недоступен для записи."
if [ "$action" = on ]; then
  if ! touch /misc/enableMTP; then
    mount -o remount,ro /misc
    fail "Не удалось включить MTP."
  fi
  if ! rm -f /misc/waitForUSB; then
    rm -f /misc/enableMTP
    mount -o remount,ro /misc
    fail "Не удалось отключить USB-ввод."
  fi
else
  if ! rm -f /misc/enableMTP; then
    mount -o remount,ro /misc
    fail "Не удалось выключить MTP."
  fi
fi
sync
mount -o remount,ro /misc || fail "Настройка записана, но MISC остался в режиме записи."
if [ "$action" = on ]; then
  show_message prntgrn "MTP включен! Перезагрузите консоль."
else
  show_message prntred "MTP выключен! Перезагрузите консоль."
fi
