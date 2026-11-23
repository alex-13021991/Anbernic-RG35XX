#!/bin/sh
cd $(dirname "$0")/ScreenshotDaemon
a=`ps | grep screenshot | grep -v grep`
if [ "$a" == "" ] ; then
    ./screenshot &
    ./printstr "L2+R2 Screenshot Ready"
    sleep 1
else
    killall screenshot
    ./printstr "    Screenshot Released    "
    sleep 1
fi
