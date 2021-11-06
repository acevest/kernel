#!/bin/bash

kpartx_out=`kpartx -l HD.IMG`
PART=`echo ${kpartx_out}|awk '{print $1}'`
PART=/dev/mapper/${PART}
LODEV=`echo ${kpartx_out}|awk '{print $5}'`
echo ${PART}
echo ${LODEV}

kpartx -u HD.IMG



mount $PART /mnt/

cp ./KERNEL.BIN /mnt/boot/Kernel
cp scripts/grub.cfg /mnt/boot/grub2/

md5sum /mnt/boot/Kernel
md5sum /mnt/boot/grub2/grub.cfg

mkdir -p /mnt/bin/
cp ./bin/shell /mnt/bin/
cp ./bin/hello /mnt/bin/
md5sum /mnt/bin/*

umount /mnt/


kpartx -d ${LODEV}
losetup -d ${LODEV}
