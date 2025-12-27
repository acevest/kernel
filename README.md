# KERNEL

## Mac环境准备


### mac

```bash
brew install i686-elf-binutils
brew install i686-elf-gcc
brew install i386-elf-gdb
brew install qemu
```

### docker

安装 fedora docker, 注意如果是arm Mac是要指定`--platform=linux/amd64`
这个docker 主要是用来打包kernel.iso的。

```bash
docker pull --platform=linux/amd64 fedora:latest
dnf install grub2-mkrescue
# 不安装grub2-pc-modules就没有/usr/lib/grub/i386-pc/就无法正确生成iso
dnf install grub2-pc-modules
dnf install xorriso

# mke2fs
dnf install e2fsprogs.x86_64

# 以下选装
# 如果要安装 chsh
dnf install util-linux-user

dnf install gcc-x86_64-linux-gnu.x86_64
dnf install binutils-x86_64-linux-gnu.x86_64
dnf install vim
dnf install git
```

运行
```
docker run -h kernel -itv kernel:/root/kernel --privileged=true --name kernel fedora
```

然后就可以在本地用`mkiso.sh`通过指定docker container的id来生成`kernel.iso`了
