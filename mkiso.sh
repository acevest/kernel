#!/bin/bash
#
# 需要先创建一个容器 docker run -it --name kernel fedora:38_x86_64
# 并安装 grub2-mkrescue
# docker exec -it kernel bash
# dnf install grub2-tools-extra.x86_64 -y  grub2-mkrescue在这个包里
# dnf install grub2-pc.x86_64 -y  没有这个生成的 iso 文件 MBR 没有启动代码
# dnf install xorriso -y

# 兼容md5 命令
MD5=md5sum
if [[ `uname` == 'Darwin' ]]; then
    MD5="md5 -q"
fi


# 检查smkrootfs命令是否存在
# 若不存在，需要进scripts/mkrootfs手动编译一个
if ! type mkrootfs >/dev/null 2>&1; then
    echo "mkrootfs command not found."
    exit 1
fi


# 找到容器
if [ $# -ne 1 ]; then
    docker ps -a
    echo "input containerid "
    read CONTAINER_ID
else
    CONTAINER_ID=$1
fi

echo "container id ${CONTAINER_ID}"

mkrootfs -name rootfs -path initrd

grub2_boot_dir="/tmp/iso/boot"
docker exec -it $CONTAINER_ID rm -rf /tmp/iso
docker exec -it $CONTAINER_ID rm -rf /tmp/kernel.iso
docker exec -it $CONTAINER_ID mkdir -p $grub2_boot_dir/grub/


# 指定要拷贝的文件和目标文件路径及名字
files[0]="KERNEL.ELF:$grub2_boot_dir/Kernel"
files[1]="scripts/iso.grub.cfg:$grub2_boot_dir/grub/grub.cfg"
files[2]="rootfs:$grub2_boot_dir/rootfs"


for i in "${!files[@]}"; do
    file_line="${files[$i]}"

    IFS=':' read -ra parts <<< "${file_line}"
    src_file="${parts[0]}"
    dst_file="${parts[1]}"
    src_file_md5=$($MD5 "$src_file" | awk '{ print $1 }')

    docker cp $src_file $CONTAINER_ID:$dst_file
    echo "$src_file $src_file_md5"
    docker exec -it $CONTAINER_ID md5sum $dst_file
done

#docker exec -it $CONTAINER_ID /usr/bin/grub2-mkrescue -o /tmp/kernel.iso /tmp/iso/
docker exec -it $CONTAINER_ID bash -c "cd /tmp && grub2-mkrescue -o kernel.iso /tmp/iso/"


docker cp $CONTAINER_ID:/tmp/kernel.iso .

$MD5 kernel.iso
