qemu-system-i386 -s -S -drive file=HD.IMG,format=raw,index=0,media=disk -drive file=kernel.iso,index=1,media=cdrom &
pid=$!
echo "pid is ${pid}"

i386-elf-gdb -x gdbscript; kill -9 $pid


# qemu-system-i386 -drive file=HD.IMG,format=raw,index=0,media=disk -cdrom kernel.iso


# -cdrom kernel.iso
# -drive file=kernel.iso,index=2,media=cdrom

# connect a CDROM to the slave of ide0
# -drive if=ide,index=1,media=cdrom

# -hda,-hdb,-hdc,-hdd
# -drive file=file,index=0,media=disk
# -drive file=file,index=1,media=disk
# -drive file=file,index=2,media=disk
# -drive file=file,index=3,media=disk


# -s shorthand for -gdb tcp::1234
# -S freeze CPU at startup (use 'c' to start execution)
