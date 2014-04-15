############################################################################
CF		= -c -I include -fno-builtin
CC		= $(_CC) $(CF) -o
SYSTEMMAP	= System.map
KERNELBIN	= KERNEL.BIN
LINKSCRIPT	= scripts/link.ld
OBJS =	boot/multiboot.S.o boot/boot.o boot/reboot.S.o	boot/cmdline.o\
	setup/*.o mm/*.o kernel/*.o fs/*.o pci/*.o lib/*.o \
	drivers/*.o
#BINS#######################################################################
$(KERNELBIN):
	cd boot && make
	cd setup && make
	cd mm && make
	cd kernel && make
	cd fs && make
	cd pci && make
	cd lib && make
	cd drivers && make
	ld -M -T$(LINKSCRIPT) -o $@ $(OBJS) > $(SYSTEMMAP)
	ld -T$(LINKSCRIPT) -o $@ $(OBJS)
	cd bin && make
	md5sum $(KERNELBIN)
	#./scripts/grub2-debug.sh
	#mv $@ KRNL.ELF
.PHONY:real
real:

.PHONY:c
c:
	rm -f $(KERNELBIN)

.PHONY:clean
clean:
	cd boot && make clean
	cd setup && make clean
	cd mm && make clean
	cd kernel && make clean
	cd fs && make clean
	cd pci && make clean
	cd lib && make clean
	cd drivers && make clean
	cd bin && make clean
	rm -f $(KERNELBIN) System.map snapshot.txt log.txt
	rm -f KRNL.ELF

.PHONY:install
install:
	cp KERNEL.BIN /boot/
.PHONY:copy
copy:
	./scripts/copy.sh
