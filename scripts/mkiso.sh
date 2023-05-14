# 在父目录运行
mkdir -p /tmp/iso/boot/grub/
cp iso.grub.cfg /tmp/iso/boot/grub/
cp KERNEL.BIN /tmp/iso/boot/Kernel
grub2-mkrescue -o kernel.iso /tmp/iso
