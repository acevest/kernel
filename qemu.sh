#!/bin/bash
#
echo "------------------"
echo `md5 kernel.iso`
echo "------------------"
# 检查serial_monitor进程是否在运行
process_name="serial_monitor"
if ! pgrep -x "$process_name" > /dev/null
then
    echo "process $process_name is not running."
    exit 1
fi

echo "process $process_name is running."


# 使用set -m来启用作业控制，以便在后台启动 QEMU 进程。
set -m

qemu-system-i386 \
    -boot d \
    -m 128\
    -smp 2 \
    -cpu qemu32,+x2apic \
    -machine q35 \
    -serial tcp::6666,server,nowait \
    -drive file=sata.img,format=raw,if=none,id=sata-disk \
    -drive file=sata2.img,format=raw,if=none,id=sata-disk2 \
    -drive file=kernel.iso,format=raw,if=none,id=atapi-cdrom \
    -device ahci,id=ahci0 \
    -device ide-hd,drive=sata-disk,bus=ahci0.0 \
    -device ide-cd,drive=atapi-cdrom,bus=ahci0.1 \
    -device ide-hd,drive=sata-disk2,bus=ahci0.5 \
    -name kernel \
    -vga std \
    -display cocoa \
    -monitor unix:/tmp/qemu-monitor.sock,server,nowait \
    -s -S \
    &

    # nc -U /tmp/qemu-monitor.sock

    # -d int,cpu_reset \
    # -monitor unix:/tmp/qemu-monitor.sock,server,nowait \
    # -machine pc-q35-9.2  \
    # -cpu qemu32,+apic \
    # -cpu qemu32,+x2apic \
    #-cpu core2duo-v1,+apic \
    #-drive file=HDb.IMG,format=raw,index=2,media=disk \
    #-cpu qemu32,+apic \
    #-cpu core2duo-v1,+apic \

    # -serial file:serial_output.log \
    # -serial tcp::6666,server,nowait \

pid=$!
echo "pid is ${pid}"

# 然后，使用set +m禁用作业控制，以便在按下 CTRL+C 时，信号不会传播到 QEMU 进程。
# 因为在gdb里调试时经常按下 CTRL+C
set +m

# 使用了一个子shell（由圆括号括起来的部分），并在子shell中使用trap命令忽略SIGINT信号
# 这样，在子shell中运行的i386-elf-gdb将接收到CTRL+C信号，但不会影响脚本的其他部分
# 当 bash 执行到圆括号内的代码时，它会等待圆括号内的所有命令执行完毕，然后才会继续执行后续的代码。
# 圆括号内的命令在子 shell 中顺序执行，因此父 shell（即脚本的主体）会等待子 shell 完成。
# 因此当i386-elf-gdb退出时，脚本将继续执行后续代码
#(
#    trap '' SIGINT
#    i386-elf-gdb KERNEL.ELF -x gdbscript
#)

i386-elf-gdb KERNEL.ELF -x gdbscript

kill -9 $pid
echo "kill pid ${pid}"
