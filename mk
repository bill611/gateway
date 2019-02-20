#!/bin/bash
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
	fi
fi
