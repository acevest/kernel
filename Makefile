OS := $(shell uname -s)
CPU_ARCH := $(shell uname -p)

ifeq ($(OS), Darwin)
	# MacOS下安装i686-elf-*的方法: brew install i686-elf-binutils
	# Apple Silicon
	ifeq ($(CPU_ARCH), arm)
		CROSS_PREFIX = i686-elf-
	# Intel MacOS
	else ifeq ($(CPU_ARCH), i386)
		CROSS_PREFIX = i686-elf-
	endif
else ifeq ($(OS), Linux)
	# Apple Silicon Docker Linux
	ifeq ($(CPU_ARCH), aarch64)
		CROSS_PREFIX = x86_64-linux-gnu-
	endif
endif


CC			= $(CROSS_PREFIX)gcc
LD			= $(CROSS_PREFIX)ld

CFLAGS		= -g -c -fno-builtin -m32 -DBUILDER='"$(shell whoami)"'
# 指示编译器生成不依赖位置无关代码
CFLAGS     += -fno-pic
# 指示编译器在生成目标文件时不省略函数调用栈帧指针: frame pointer
CFLAGS     += -fno-omit-frame-pointer
# 禁用控制流保护: Control-Flow Enforcement Technology (CET)
CFLAGS     += -fcf-protection=none
CFLAGS     += -DNR_TTYS=3
CFLAGS     += -DFIXED_SYSENTER_ESP_MODE=1
CFLAGS     += -DENABLE_BOOT_WAIT=0
CFLAGS     += -DENABLE_CLOCK_IRQ_WAIT=0

SYSTEMMAP	= System.map
KERNELBIN	= KERNEL.ELF
LINKSCRIPT	= scripts/link.ld

SRC_DIRS = boot mm lib fs kernel drivers
INC_DIRS = include drivers boot mm fs

CFLAGS += ${INC_DIRS:%=-I%}

SOURCE_FILES := $(foreach DIR, $(SRC_DIRS), $(wildcard $(DIR)/*.[cS]))
HEADER_FILES := $(foreach DIR, $(INC_DIRS), $(wildcard $(DIR)/*.h))

OBJS := $(patsubst %,%.o,$(SOURCE_FILES))

${KERNELBIN}: ${OBJS}
	${LD} -z noexecstack -m elf_i386 -M -T$(LINKSCRIPT) $(OBJS) -o $@ > $(SYSTEMMAP)
	nm -a $@ > kernel.sym
	rm kernel/setup.c.o

%.S.o: %.S ${HEADER_FILES}
	${CC} ${CFLAGS} $< -o $@

%.c.o: %.c ${HEADER_FILES}
	${CC} ${CFLAGS} $< -o $@

.PHONY: c
c:
	rm -f $(KERNELBIN)

.PHONY: clean $(SRC_DIRS)
clean: $(SRC_DIRS)
	rm -f $(KERNELBIN) $(SYSTEMMAP)

$(SRC_DIRS):
	@echo "clean *.o files in $@"
	@find $@ -name "*\.o" -delete


.PHONY: install
install:
	cp -p KERNEL.ELF /boot/
	sync
	md5sum /boot/KERNEL.ELF
	md5sum KERNEL.ELF
	mkdir -p /kernel/bin/
	cp bin/hello /kernel/bin/
	cp bin/shell /kernel/bin/

.PHONY: cp
cp:
	cd bin && make clean
	cd bin && make
	./scripts/copy.sh
