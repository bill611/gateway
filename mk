#!/bin/bash
set -e

if [ "$#" -lt 1 ]; then
	echo 'please input version like "./mk 1 or 2"'
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
fi

# make clean -s

