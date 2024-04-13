#!/bin/bash
# 在父目录运行
# 这个代码必需要在x86的linux机器上运行
# 因为如果在其它机器上运行，其grub就不是x86版本
source ~/.bashrc
rm -rf /tmp/iso
mkdir -p /tmp/iso/boot/grub/
sync



copy_file_with_retry() {
  local expected_md5="$1"
  local src_file="$2"
  local dst_file="/tmp/iso/boot/$3"
  local max_retries=$4
  local retry_count=0


  while [ $retry_count -lt $max_retries ]; do
    #  查看源文件的大小是不是不为0
    if [ -s $src_file ]; then
      cp -f $src_file $dst_file

      #  拷贝后再校验 md5 如果不一致继续重试
      local dst_file_md5=$(md5sum $dst_file | awk '{ print $1 }')

      echo "file: $src_file md5: ${dst_file_md5} expected md5: $expected_md5"

      if [ "$expected_md5" == "$dst_file_md5" ]; then
        echo "File $src_file copied successfully to $dst_file."
        return 0
      else
        echo "retry"
      fi
    fi

    # 等1秒再重试
    sleep 1
    ((retry_count++))
  done

  echo "failed to copy file $src_file to $dst_file after $max_retries attempts."
  return 1
}



for arg in "$@"
do
  IFS=':' read -ra file_line <<< "$arg"

  md5sum=${file_line[0]}
  src_file=${file_line[1]}
  dst_file=${file_line[2]}
  copy_file_with_retry "${md5sum}" "${src_file}" "${dst_file}" 3
done



#copy_file_with_retry "KERNEL.ELF" "/tmp/iso/boot/Kernel"
#copy_file_with_retry "scripts/iso.grub.cfg" "/tmp/iso/boot/grub/grub.cfg"
#cp -f scripts/iso.grub.cfg /tmp/iso/boot/grub/grub.cfg
#ls -l KERNEL.ELF
#cp -f KERNEL.ELF /tmp/iso/boot/Kernel
#md5sum KERNEL.ELF
#md5sum /tmp/iso/boot/Kernel
mkrootfs -path initrd --name rootfs
cp -f rootfs /tmp/iso/boot/
grub2-mkrescue -o kernel.iso /tmp/iso/
md5sum kernel.iso
sync

