#!/bin/sh
cd $(dirname "$0")/ScreenshotDaemon
a=`ps | grep screenshot | grep -v grep`
if [ "$a" == "" ] ; then
    ./screenshot &
    ./printstr "截屏已开启: L2+R2"
    sleep 1
else
    killall screenshot
    ./printstr "   截屏已关闭   "
    sleep 1
fi
