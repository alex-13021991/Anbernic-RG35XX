#!/bin/sh
cd "$(dirname "$0")/Toggle USB"

if [ ! -f /misc/waitForUSB ]; then

  mount -o remount,rw /misc
  touch /misc/waitForUSB
  rm -f /misc/enableMTP
 
  if [ -f /misc/enableADB ]; then
    rm /misc/enableADB
   fi
  mount -o remount,ro /misc

  echo 50 > "/sys/class/power_supply/battery/moto"
  busybox sleep 0.1
  echo 0 > "/sys/class/power_supply/battery/moto"

  ./prntgrn "   USB-ввод включен! Перезагрузите консоль.   "
 
else

  mount -o remount,rw /misc
  rm /misc/waitForUSB
  mount -o remount,ro /misc

  echo 50 > "/sys/class/power_supply/battery/moto"
  busybox sleep 0.1
  echo 0 > "/sys/class/power_supply/battery/moto"

  ./prntred "   USB-ввод выключен! Перезагрузите консоль.   "

fi

sleep 2
