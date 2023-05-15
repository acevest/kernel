OS := $(shell uname -s)
CPU_ARCH := $(shell uname -p)

CC			= gcc
LD			= ld
ifeq ($(OS), Darwin)
	# MacOS下安装i686-elf-*的方法: brew install i686-elf-binutils
	# Apple Silicon
	ifeq ($(CPU_ARCH), arm)
		CC		= i686-elf-gcc
		LD		= i686-elf-ld
	# Intel MacOS
	else ifeq ($(CPU_ARCH), i386)
		CC		= i686-elf-gcc
		LD		= i686-elf-ld
	endif
else ifeq ($(OS), Linux)
	# Apple Silicon Docker Linux
	ifeq ($(CPU_ARCH), aarch64)
		CC		= x86_64-linux-gnu-gcc
		LD		= x86_64-linux-gnu-ld
	endif
endif


CFLAGS		= -g -c -fno-builtin -m32 -DBUILDER='"$(shell whoami)"'
SYSTEMMAP	= System.map
KERNELBIN	= KERNEL.BIN
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

c:
	rm -f $(KERNELBIN)

clean:
	rm -f $(OBJS)
	rm -f $(KERNELBIN) $(SYSTEMMAP)

install:
	cp -p KERNEL.BIN /boot/
	sync
	md5sum /boot/KERNEL.BIN
	md5sum KERNEL.BIN
	mkdir -p /kernel/bin/
	cp bin/hello /kernel/bin/
	cp bin/shell /kernel/bin/
cp:
	cd bin && make clean
	cd bin && make
	./scripts/copy.sh
