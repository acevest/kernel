#!/bin/bash
set -x

HDIMG=out.img
TMP=build/tmp
MNT=$TMP/mnt

mkdir -p ${MNT}


# 生成硬盘
dd if=/dev/zero of=$HDIMG bs=512 count=24576


# 挂载到loop设备
lodev=`losetup -f --show $HDIMG`
echo "(hd0) ${lodev}" > ${TMP}/device.map
sleep 1

# 创建主分区并设置为活动分区
parted -s ${lodev} mklabel msdos
parted -s ${lodev} mkpart primary ext2 200k 100% -a minimal
parted ${lodev} set 1 boot on
sleep 1

ls -l ${lodev}*

# 在分区上创建ext2文件系统
PART=${lodev}p1
mke2fs -b 4096 $PART
sleep 1


# 挂载文件系统
mkdir -p $MNT
mount $PART $MNT
sleep 1

# 创建目录
mkdir -p ${MNT}/boot/grub2
mkdir -p ${MNT}/bin/

# 安装grub
grub2-install --no-floppy \
              --grub-mkdevicemap=${TMP}/device.map 			\
	      --target=i386-pc              				\
              --modules="biosdisk part_msdos ext2 configfile normal multiboot multiboot2"     \
              --boot-directory=$MNT/boot  \
              ${lodev}

# 拷贝grub配置文件
cp grub.cfg ${MNT}/boot/grub2/
sleep 1

# 拷贝内核文件
cp ../KERNEL.ELF ${MNT}/boot/Kernel
sleep 1

umount $MNT
sleep 1

partx -d ${lodev}
sleep 1

losetup -d ${lodev}
