MAKEROOT = $(shell pwd)

CFLAGS =
ifeq ($(PLATFORM), NU)
	PREFIX =$(NUVOTON_CROOS_PATH)/bin/arm-none-linux-gnueabi-
	CFLAGS += -DNUVOTON
	LINKFLAGS = -muclibc
endif

ifeq ($(PLATFORM), AK)
	PREFIX =$(ANYKA_CROOS_PATH)/bin/arm-none-linux-gnueabi-
	CFLAGS += -DANYKA
	LINKFLAGS = 
endif

ifeq ($(PLATFORM), PC)
	PREFIX =
	CFLAGS += -DX86
	LINKFLAGS = 
endif

VERSION =
SDK10_PATH =sdk/1.0
SDK20_PATH =sdk/2.0/ilop-sdk-6aad4ee
SDK23_PATH =sdk/2.3/iotkit-embedded-2.3.0

# 主程序Makefile

# 填相关编译路径
KERNERL_PATH=
# 在指定目录下生成的应用程序
EXE = gw
BIN_TARGET = ${BIN_DIR}/${EXE}

SRC_DIR = $(MAKEROOT)/src
OBJ_DIR = $(MAKEROOT)/obj
BIN_DIR = $(MAKEROOT)/out/bin

CC =$(PREFIX)gcc
STRIP = $(PREFIX)strip $(BIN_TARGET)

# INC_DIR 目录下为相应库的头文件
INC_DIR = \
		  $(KERNERL_PATH)/include \
		  $(MAKEROOT)/src\
		  $(MAKEROOT)/include\
		  $(MAKEROOT)/src/app\
		  $(MAKEROOT)/src/platform\
		  $(MAKEROOT)/src/product\
		  $(MAKEROOT)/src/wireless\
		  $(MAKEROOT)/src/drivers\

SRC = \
		$(wildcard ${SRC_DIR}/app/*.c) \
		$(wildcard ${SRC_DIR}/wireless/*.c) \
		$(wildcard ${SRC_DIR}/drivers/*.c) \
		$(wildcard ${SRC_DIR}/drivers/iniparser/*.c)

ifeq ($(VERSION), 1)
	INC_DIR += $(MAKEROOT)/include/v1.0

	LIB_DIR += $(MAKEROOT)/lib/v1.0 $(MAKEROOT)/lib/libs/libs $(MAKEROOT)/$(SDK10_PATH)/lib

	SRC += $(wildcard ${SRC_DIR}/platform/*.c) \
		   $(wildcard ${SRC_DIR}/product/*.c)

	XLINKER = -Xlinker "-(" -lsqlite3 -lresolv -lm -lssl -lcrypto -lalink_agent -lpthread -ldl -lcjson -Xlinker "-)"
	CFLAGS += -DV1
	CP_TARGET = $(MAKEROOT)/../nand/v1.0/nand1-2/
endif

ifeq ($(VERSION), 2)
	INC_DIR += $(MAKEROOT)/$(SDK20_PATH)/include \
			   $(MAKEROOT)/src/hal

	LIB_DIR += $(MAKEROOT)/lib/v2.0 $(MAKEROOT)/lib/libs/libs $(MAKEROOT)/$(SDK20_PATH)/lib

	SRC += $(wildcard ${SRC_DIR}/hal/*.c)
	XLINKER = -Xlinker "-(" -lsqlite3 -lm -lilop-tls -lilop-sdk -lpthread -ldl -lrt -Xlinker "-)"
	CFLAGS += -DV2
	CP_TARGET = $(MAKEROOT)/../nand/v2.0/nand1-2/
endif

ifeq ($(VERSION), 23)
	INC_DIR += $(MAKEROOT)/$(SDK23_PATH)/output/release/include 

	LIB_DIR += $(MAKEROOT)/lib/v2.3 $(MAKEROOT)/$(SDK23_PATH)/output/release/lib 

	XLINKER = -Xlinker "-(" -lsqlite3 -lm -liot_hal -liot_tls -liot_sdk -lpthread -ldl -lrt -Xlinker "-)"
	CFLAGS += -DV2 -DV23
	CP_TARGET = $(MAKEROOT)/../nand/v2.3/bin/
endif

ifeq ($(DBG), 1)
	CFLAGS += -g -O0 -DWATCHDOG_DEBUG
	LINKFLAGS += -g -O0
	LIB_DIR += $(MAKEROOT)/lib/x86
	CP_TARGET := ${HOME}/arm_share/ankgw
else
	CFLAGS += -O2
endif


CFLAGS += -D_PLATFORM_IS_LINUX_ -D_GNU_SOURCE ${addprefix -I,${INC_DIR}}


# wildcard:扩展通配符，notdir;去除路径，patsubst;替换通配符

OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${SRC}))
DEPS = $(patsubst %.c, ${OBJ_DIR}/%.d, $(notdir ${SRC}))

# 链接路径
# -Xlinker编译时可重复查找依赖库，和库的次序无关
# LIB_DIR =



export CC LIB_DIR CFLAGS OBJ_DIR INC_DIR DEPS VERSION
# $@：表示目标文件，$^：表示所有的依赖文件，$<：表示第一个依赖文件，$?：表示比目标还要新的依赖文件列表
all: make_C ${BIN_TARGET}
	cp -u ${BIN_TARGET} ${CP_TARGET}

make_C:
	@mkdir -p ${OBJ_DIR}
	@make -C src

# 在指定目录下，将所有的.c文件编译为相应的同名.o文件
${BIN_TARGET}:${OBJ}
	@$(CC) $(LINKFLAGS) -o $@ $(OBJ) ${addprefix -L,${LIB_DIR}} ${XLINKER}
	@${STRIP} $@

debug:
	make -C src debug

.PHONY:clean
clean:
	@-rm -rf ${BIN_TARGET} obj* ${BIN_DIR}
