CC = gcc
CFLAGS = -c -I../include -fno-builtin
LDOBJS = ../lib/errno.o ../lib/fork.o ../lib/lib.o ../lib/open.o \
	../lib/stat.o ../lib/string.o ../lib/write.o ../lib/exec.o \
	../lib/vsprintf.o ../lib/read.o \
	../lib/keyboard.o ../lib/exit.o
all:
	gcc -c shell.S -o shell.S.o
	gcc $(CFLAGS) shell.c -o shell.o
	ld $(LDOBJS) shell.S.o shell.o -o sh
	gcc -c helloworld.S -o helloworld.S.o
	gcc $(CFLAGS) helloworld.c -o helloworld.o
	ld $(LDOBJS) helloworld.S.o helloworld.o -o hw 

.PHONY:clean
clean:
	rm sh hw helloworld.o -f
.PHONY:c
c:
	make clean
