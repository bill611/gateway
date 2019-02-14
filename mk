#!/bin/bash
set -e

if [ "$#" -lt 1 ]; then
	echo 'please input version like "./mk 1(1.0) , 2(2.0) or 23(2.3)"'
	exit
elif [ "$1" -eq 1 ]; then
	echo '
COMPILE = $(CC) -muclibc -g -o $@ $(OBJ) ${addprefix -L,${LIB_DIR}} ${XLINKER}
CFLAGS += -muclibc -g -c -DWATCHDOG_DEBUG -O0
CP_TARGET = cp -u ${BIN_TARGET} ~/arm_share/' > evn.mk
make VERSION=1

elif [ "$1" -eq 2 ]; then
	echo '
COMPILE = $(CC) -muclibc -g -o $@ $(OBJ) ${addprefix -L,${LIB_DIR}} ${XLINKER}
CFLAGS += -muclibc -g -c -DWATCHDOG_DEBUG -O0 -D_PLATFORM_IS_LINUX_  -D_GNU_SOURCE
CP_TARGET = cp -u ${BIN_TARGET} ~/arm_share/' > evn.mk
make VERSION=2

elif [ "$1" -eq 23 ]; then
	echo '
COMPILE = $(CC) -g -o $@ $(OBJ) ${addprefix -L,${LIB_DIR}} ${XLINKER}
CFLAGS += -g -c -DWATCHDOG_DEBUG -O0 -D_PLATFORM_IS_LINUX_  -D_GNU_SOURCE
CP_TARGET = cp -u ${BIN_TARGET} ~/arm_share/' > evn.mk

if [ "$2" == "g" ]; then
	make VERSION=23 DBG=1
else
	make VERSION=23
fi

# make clean -s

fi
