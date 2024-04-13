#!/bin/bash

# 兼容md5 命令
MD5=md5sum
if [[ `uname` == 'Darwin' ]]; then
    MD5="md5 -q"
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
sync



# 指定要拷贝的文件和目标文件路径及名字
files[0]="KERNEL.ELF:Kernel"
files[1]="scripts/iso.grub.cfg:grub/grub.cfg"

declare -a params

for i in "${!files[@]}"; do
    file_line="${files[$i]}"
    
    IFS=':' read -ra parts <<< "${file_line}"
    src_file="${parts[0]}"
    src_file_md5=$($MD5 "$src_file" | awk '{ print $1 }')

    s="$src_file_md5:$file_line"

    params[$i]="$s"

    echo "$s"
done

#echo ${params[@]}

params_line="${params[@]}"
docker exec -it $CONTAINER_ID /bin/bash -c "cd /root/workspace/kernel && ./scripts/mkiso.sh ${params_line}"

sync

$MD5 kernel.iso
