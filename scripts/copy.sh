#!/bin/bash
#set -x

HDIMG=./HD.IMG
lodev=`losetup -f --show $HDIMG`

partx -a ${lodev}

PART=${lodev}p1

mount $PART /mnt/

cp ./KERNEL.ELF /mnt/boot/Kernel
cp scripts/grub.cfg /mnt/boot/grub2/

md5sum /mnt/boot/Kernel

mkdir -p /mnt/bin/
cp ./bin/shell /mnt/bin/
cp ./bin/hello /mnt/bin/
md5sum /mnt/bin/*

umount /mnt/

partx -d ${lodev}p1

losetup -d ${lodev}
