.extern main
.global _start

_start:
    nop
    nop
    nop
    nop
    nop
    nop

    pushl   $0  # env
    pushl   $0  # argv
    pushl   $1  # argc

    call main


    add $12, %esp

    nop
    nop
    nop
    nop
    nop
    nop
    nop
1:
    jmp 1b
