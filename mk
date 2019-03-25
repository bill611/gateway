#!/bin/bash
# TOOLS_PATH="/home/xubin/work/tools/platform/anyka/platform/rootfs"    
source path
mkdir -p update/
mkdir -p out/bin
set -e

if [ "$#" -lt 1 ]; then
	echo 'please input version like "./mk 1(1.0) , 2(2.0) or 23(2.3)"'
	exit
fi

if [ "$1" -eq 1 ]; then
	make VERSION=1 PLATFORM=NU DBG=1
elif [ "$1" -eq 2 ]; then
	make VERSION=2 PLATFORM=NU DBG=1
elif [ "$1" -eq 23 ]; then
	if [ "$2" == "g" ]; then
		make VERSION=23 PLATFORM=PC DBG=1
	else
		make VERSION=23 PLATFORM=AK DBG=1
		if [ "$?" == 0 ]; then
			$TOOLS_PATH/mksquashfs out/bin update/bin.sqsh4 -noappend > /dev/null
			echo "Debug build finished!"
		else
			echo "make error!"
		fi
		echo "  "
		echo "tar czf Update.tar.gz update"
		tar czf Update.tar.gz update/
		mv Update.tar.gz Update.cab
		mv Update.cab out/
	fi
fi
