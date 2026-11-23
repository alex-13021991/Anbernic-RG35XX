#!/bin/sh
progdir=`dirname "$0"`
case "$1" in
	1)
		rm -f $progdir/../Screenshot_Daemon.sh
		cp -f $progdir/截屏.sh $progdir/../
		sync
		;;
	2)
		rm -f $progdir/../截屏.sh
		cp -f $progdir/Screenshot_Daemon.sh $progdir/../
		sync
		;;
esac
