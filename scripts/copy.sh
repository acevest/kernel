#!/bin/bash
#set -x

HDIMG=./HD.IMG
lodev=`losetup -f --show $HDIMG`

partx -a ${lodev}

PART=${lodev}p1

mount $PART /mnt/

cp ./KERNEL.BIN /mnt/boot/Kernel
cp scripts/grub.cfg /mnt/boot/grub2/

md5sum /mnt/boot/Kernel

mkdir -p /mnt/bin/
cp ./bin/hw /mnt/bin/
cp ./bin/sh /mnt/bin/

umount /mnt/

partx -d ${lodev}p1

losetup -d ${lodev}
