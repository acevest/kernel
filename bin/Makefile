CC = gcc
CFLAGS = -c -I../include -fno-builtin
LDOBJS = ../lib/errno.c.o ../lib/fork.c.o ../lib/lib.c.o ../lib/open.c.o \
	../lib/stat.c.o ../lib/string.c.o ../lib/write.c.o ../lib/exec.c.o \
	../lib/vsprintf.c.o ../lib/read.c.o \
	../lib/keyboard.c.o ../lib/exit.c.o ../lib/syscall.c.o
all:
	gcc -c shell.S -o shell.S.c.o
	gcc $(CFLAGS) shell.c -o shell.c.o
	ld $(LDOBJS) shell.S.c.o shell.c.o -o sh
	gcc -c helloworld.S -o helloworld.S.c.o
	gcc $(CFLAGS) helloworld.c -o helloworld.c.o
	ld $(LDOBJS) helloworld.S.c.o helloworld.c.o -o hw 

.PHONY:clean
clean:
	rm sh hw helloworld.c.o -f
.PHONY:c
c:
	make clean
