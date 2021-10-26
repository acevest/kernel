#!/bin/bash
set -x

HDIMG=out.img
TMP=build/tmp
MNT=$TMP/mnt

dd if=/dev/zero of=$HDIMG bs=512 count=49152

lodev=`losetup -f --show $HDIMG`

sleep 1

parted ${lodev} mklabel msdos

sleep 1

parted ${lodev} mkpart primary ext2 200k 100% -a minimal

sleep 1

parted ${lodev} set 1 boot on

sleep 1

#partx -a ${lodev}

#sleep 1

ls -l ${lodev}*

PART=${lodev}p1

sleep 1

mke2fs -b 4096 $PART

sleep 1

mkdir -p $MNT

mount $PART $MNT

sleep 1

echo "(hd0) ${lodev}" > ${TMP}/device.map

mkdir -p ${MNT}/boot/grub2/

grub2-install --target=i386-pc              \
              --grub-mkdevicemap=${TMP}/device.map                                  \
              --modules="biosdisk part_msdos ext2 configfile normal multiboot"      \
              --root-directory=$MNT \
              ${lodev}

cp grub.cfg ${MNT}/boot/grub2/
cp ../KERNEL.BIN ${MNT}/boot/Kernel

mkdir -p ${MNT}/bin/

sleep 1

umount $MNT

sleep 1

partx -d ${lodev}

sleep 1

losetup -d ${lodev}
