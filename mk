#!/bin/bash
set -e

if [ "$#" == 0 ]; then
	echo 'CC = arm-linux-gcc
STRIP = arm-linux-strip $(BIN_TARGET)
LIB_DIR += $(MAKEROOT)/lib $(MAKEROOT)/libs/libs $(MAKEROOT)/sdk/lib
COMPILE = $(CC) -muclibc -g -o $@ $(OBJ) ${addprefix -L,${LIB_DIR}} ${XLINKER}
CFLAGS = -muclibc -g -c -DWATCHDOG_DEBUG -O0
CP_TARGET = cp -u ${BIN_TARGET} ~/arm_share/' > evn.mk

else
	echo 'CC = gcc
STRIP=
LIB_DIR += $(MAKEROOT)/lib_x86
COMPILE = $(CC) -g -o $@ $(OBJ) ${addprefix -L,${LIB_DIR}} ${XLINKER}
CFLAGS = -g -DPC -DWATCHDOG_DEBUG -c -O0
CP_TARGET =' > evn.mk
fi

# make clean -s
make

