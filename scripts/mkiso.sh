#!/bin/bash
# 在父目录运行
# 这个代码必需要在x86的linux机器上运行
# 因为如果在其它机器上运行，其grub就不是x86版本
source ~/.bashrc
mkdir -p /tmp/iso/boot/grub/
cp scripts/iso.grub.cfg /tmp/iso/boot/grub/grub.cfg
cp KERNEL.ELF /tmp/iso/boot/Kernel
mkrootfs -path initrd --name rootfs
cp rootfs /tmp/iso/boot/
grub2-mkrescue -o kernel.iso /tmp/iso/
