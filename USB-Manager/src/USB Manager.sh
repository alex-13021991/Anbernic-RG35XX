#!/bin/sh
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/USBManager" && pwd) || exit 1
cd "$APP_DIR" || exit 1
if [ -f app.log ]; then mv -f app.log app-previous.log; fi
exec >app.log 2>&1
echo "USB Manager 0.1.0-test — GarlicOS 1.4.9"
for node in /sys/class/android_usb/android0/enable /sys/class/android_usb/android0/functions /sys/class/android_usb/android0/state /sys/monitor/usb_port/config/run /sys/monitor/usb_port/config/idpin_debug /sys/monitor/usb_port/config/usb_con_msg; do
  if [ -r "$node" ]; then printf '%s: ' "$node"; cat "$node"; printf '\n'; fi
done
export SDL_NOMOUSE=1
LD_PRELOAD="$APP_DIR/j2k.so" ./usb-manager
rc=$?
echo "Exit: $rc"
exit "$rc"
