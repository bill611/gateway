MAKEROOT = $(shell pwd)

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
# INC_DIR 目录下为相应库的头文件
INC_DIR = \
		  $(KERNERL_PATH)/include \
		  $(MAKEROOT)/include \
		  $(MAKEROOT)/src\
		  $(MAKEROOT)/src/app\
		  $(MAKEROOT)/src/platform\
		  $(MAKEROOT)/src/product\
		  $(MAKEROOT)/src/wireless\


include evn.mk

CFLAGS += ${addprefix -I,${INC_DIR}}

SRC = $(wildcard ${SRC_DIR}/platform/*.c)
SRC += $(wildcard ${SRC_DIR}/product/*.c)
SRC += $(wildcard ${SRC_DIR}/app/*.c)
SRC += $(wildcard ${SRC_DIR}/wireless/*.c)

# wildcard:扩展通配符，notdir;去除路径，patsubst;替换通配符

OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${SRC}))
DEPS = $(patsubst %.c, ${OBJ_DIR}/%.d, $(notdir ${SRC}))

# 链接路径
# -Xlinker编译时可重复查找依赖库，和库的次序无关
# LIB_DIR = 

XLINKER = -Xlinker "-(" -lm -lssl -lcrypto -lalink_agent -lpthread -ldl -Xlinker "-)"


export CC LIB_DIR CFLAGS OBJ_DIR INC_DIR DEPS
# $@：表示目标文件，$^：表示所有的依赖文件，$<：表示第一个依赖文件，$?：表示比目标还要新的依赖文件列表
all: make_C ${BIN_TARGET} 
	${CP_TARGET}	

make_C:
	@mkdir -p ${OBJ_DIR}
	@make -C src 

# 在指定目录下，将所有的.c文件编译为相应的同名.o文件
${BIN_TARGET}:${OBJ}
	@$(COMPILE)

debug:
	make -C src debug

.PHONY:clean
clean:
	@-rm -rf ${BIN_TARGET} ${OBJ_DIR}
