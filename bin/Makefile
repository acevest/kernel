CC = gcc
LDFLAG = -e _start
CFLAGS = -c -I../include -fno-builtin
LIBC_DIRS = ../lib
LIBC_SRCS := $(foreach DIR, $(LIBC_DIRS), $(wildcard $(DIR)/*.[cS]))
LIBC_OBJS := $(patsubst %,%.o,$(LIBC_SRCS))
BINS := shell hello

all:$(BINS)

shell: shell.o
	gcc $(CFLAGS) shell.c -o shell.o
	ld ${LIBC_OBJS} shell.o -o shell
hello: hello.o
	gcc $(CFLAGS) hello.c -o hello.o
	ld $(LIBC_OBJS) hello.o -o hello


.PHONY:clean
clean:
	rm $(BINS) *.o -f
.PHONY:c
c:
	make clean
