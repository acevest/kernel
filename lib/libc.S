.extern main
.global _start

_start:
    nop
    nop
    nop
    nop
    nop
    nop

/*
    movl $11, %eax;

    pushl $1f;
    pushl %ecx;
    pushl %edx;
    pushl %ebp;
    movl %esp,%ebp;
    sysenter;
    1:

loop:
    jmp loop
    */

    pushl   $0  # env
    pushl   $0  # argv
    pushl   $1  # argc

    call main


    addl    $12, %esp

    nop
    nop
    nop
    nop
    nop
    nop
    nop
die:
    jmp die 
