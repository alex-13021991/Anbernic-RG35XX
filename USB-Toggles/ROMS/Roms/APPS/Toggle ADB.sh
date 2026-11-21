#!/bin/sh
cd "$(dirname "$0")/Toggle USB"

if [ ! -f /misc/enableADB ]; then

  mount -o remount,rw /misc
  touch /misc/enableADB
 
  if [ -f /misc/waitForUSB ]; then
    rm /misc/waitForUSB
   fi
  mount -o remount,ro /misc

  echo 50 > "/sys/class/power_supply/battery/moto"
  busybox sleep 0.1
  echo 0 > "/sys/class/power_supply/battery/moto"

  ./prntgrn "   ADB включен! Перезагрузите консоль.   "
 
else

  mount -o remount,rw /misc
  rm /misc/enableADB
  mount -o remount,ro /misc

  echo 50 > "/sys/class/power_supply/battery/moto"
  busybox sleep 0.1
  echo 0 > "/sys/class/power_supply/battery/moto"

  ./prntred "   ADB выключен! Перезагрузите консоль.   "

fi

sleep 2
