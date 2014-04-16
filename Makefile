############################################################################
CC			= gcc
CFLAGS		= -c -fno-builtin
SYSTEMMAP	= System.map
KERNELBIN	= KERNEL.BIN
LINKSCRIPT	= scripts/link.ld
############################################################################

SRC_DIRS = boot setup mm lib fs kernel drivers pci
INC_DIRS = include drivers

CSOURCE_FILES := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
SSOURCE_FILES := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.S))

OBJS := $(patsubst %.c,%.c.o,$(CSOURCE_FILES))
OBJS += $(patsubst %.S,%.S.o,$(SSOURCE_FILES))

CFLAGS += ${INC_DIRS:%=-I%}

${KERNELBIN}: ${OBJS} $
	ld -M -T$(LINKSCRIPT) $(OBJS) -o $@ > $(SYSTEMMAP)

%.S.o: %.S
	${CC} ${CFLAGS} $< -o $@

%.c.o: %.c
	${CC} ${CFLAGS} $< -o $@

c:
	rm -f $(KERNELBIN)

clean:
	rm -f $(OBJS)
	rm -f $(KERNELBIN) $(SYSTEMMAP)

install:
	cp KERNEL.BIN /boot/

copy:
	./scripts/copy.sh
