MAKEROOT = $(shell pwd)
VERSION =
SDK_PATH =sdk/2.0/ilop-sdk-6aad4ee

# 主程序Makefile

# 填相关编译路径
KERNERL_PATH=
# 在指定目录下生成的应用程序
EXE = gw
BIN_TARGET = ${BIN_DIR}/${EXE}

SRC_DIR = $(MAKEROOT)/src
OBJ_DIR = $(MAKEROOT)/obj
BIN_DIR = $(MAKEROOT)
include path.mk

CC = arm-linux-gcc
STRIP = arm-linux-strip $(BIN_TARGET)

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
		$(wildcard ${SRC_DIR}/drivers/*.c)
CFLAGS =
ifeq ($(VERSION), 1)
	INC_DIR += $(MAKEROOT)/include/v1.0

	LIB_DIR += $(MAKEROOT)/lib $(MAKEROOT)/libs/libs $(MAKEROOT)/sdk/1.0/lib

	SRC += $(wildcard ${SRC_DIR}/platform/*.c) \
		   $(wildcard ${SRC_DIR}/product/*.c)

	XLINKER = -Xlinker "-(" -lsqlite3 -liniparser -lresolv -lm -lssl -lcrypto -lalink_agent -lpthread -ldl -lcjson -Xlinker "-)"
	CFLAGS += -DV1
endif

ifeq ($(VERSION), 2)
	INC_DIR += $(MAKEROOT)/include/v2.0 \
			   $(MAKEROOT)/src/hal

	LIB_DIR += $(MAKEROOT)/lib $(MAKEROOT)/libs/libs $(MAKEROOT)/$(SDK_PATH)/lib

	SRC += $(wildcard ${SRC_DIR}/hal/*.c)
	XLINKER = -Xlinker "-(" -lsqlite3 -liniparser -lm -lilop-tls -lilop-sdk -lpthread -ldl -lrt -Xlinker "-)"
	CFLAGS += -DV2
endif

CFLAGS += ${addprefix -I,${INC_DIR}}

include evn.mk

# wildcard:扩展通配符，notdir;去除路径，patsubst;替换通配符

OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${SRC}))
DEPS = $(patsubst %.c, ${OBJ_DIR}/%.d, $(notdir ${SRC}))

# 链接路径
# -Xlinker编译时可重复查找依赖库，和库的次序无关
# LIB_DIR =



export CC LIB_DIR CFLAGS OBJ_DIR INC_DIR DEPS VERSION
# $@：表示目标文件，$^：表示所有的依赖文件，$<：表示第一个依赖文件，$?：表示比目标还要新的依赖文件列表
all: make_C ${BIN_TARGET}
	${CP_TARGET}

make_C:
	@mkdir -p ${OBJ_DIR}
	@make -C src

# 在指定目录下，将所有的.c文件编译为相应的同名.o文件
${BIN_TARGET}:${OBJ}
	@$(COMPILE)
	@${STRIP} $@

debug:
	make -C src debug

.PHONY:clean
clean:
	@-rm -rf ${BIN_TARGET} ${OBJ_DIR}
