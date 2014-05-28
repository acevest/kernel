CC			= gcc
CFLAGS		= -g -c -fno-builtin
SYSTEMMAP	= System.map
KERNELBIN	= KERNEL.BIN
LINKSCRIPT	= scripts/link.ld

SRC_DIRS = boot setup mm lib fs kernel drivers pci
INC_DIRS = include drivers

CFLAGS += ${INC_DIRS:%=-I%}

SOURCE_FILES := $(foreach DIR, $(SRC_DIRS), $(wildcard $(DIR)/*.[cS]))
HEADER_FILES := $(foreach DIR, $(INC_DIRS), $(wildcard $(DIR)/*.h))

OBJS := $(patsubst %,%.o,$(SOURCE_FILES))

${KERNELBIN}: ${OBJS}
	ld -M -T$(LINKSCRIPT) $(OBJS) -o $@ > $(SYSTEMMAP)
	rm setup/setup.c.o

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

copy:
	./scripts/copy.sh
