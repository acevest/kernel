CC			= gcc
CFLAGS		= -g -c -fno-builtin
SYSTEMMAP	= System.map
KERNELBIN	= KERNEL.BIN
LINKSCRIPT	= scripts/link.ld

SRC_DIRS = boot setup mm lib fs kernel drivers pci
INC_DIRS = include drivers

CFLAGS += ${INC_DIRS:%=-I%}

SOURCE_FILES := $(foreach DIR, $(SRC_DIRS), $(wildcard $(DIR)/*.[cS]))
OBJS := $(patsubst %,%.o,$(SOURCE_FILES))

${KERNELBIN}: ${OBJS}
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
