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



CFLAGS := -g -c -fno-builtin -m32 -DBUILDER='"$(shell whoami)"'
CFLAGS += -m32 -c -I../include -fno-builtin
CFLAGS += -std=gnu11
LDFLAG = -e _start
LIBC_DIRS = ../lib
LIBC_SRCS := $(foreach DIR, $(LIBC_DIRS), $(wildcard $(DIR)/*.[cS]))
LIBC_OBJS := $(patsubst %,%.o,$(LIBC_SRCS))
BINS := init

all:$(BINS)

shell: shell.o
	$(CC) $(CFLAGS) shell.c -o shell.o
	$(LD)  -m elf_i386 ${LIBC_OBJS} shell.o -o shell
hello: hello.o
	$(CC) $(CFLAGS) hello.c -o hello.o
	$(LD)  -m elf_i386 $(LIBC_OBJS) hello.o -o hello

init: init.o
	$(CC) $(CFLAGS) init.S -o init.o
	#$(LD)  -m elf_i386 $(LIBC_OBJS) hello.o -o hello
	#$(LD)  -m elf_i386 init.o -o init
	#$(LD)  -m elf_i386 -Ttext 0xc017f000 --oformat binary init.o -o init
	$(LD)  -m elf_i386 -Ttext 0x0017f000 --oformat binary init.o -o init

.PHONY:clean
clean:
	rm $(BINS) *.o -f
.PHONY:c
c:
	make clean
