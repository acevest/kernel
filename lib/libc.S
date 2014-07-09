.extern main
.global _start

_start:
    nop
    nop
    nop
    nop
    nop
    nop

    call main

    nop
    nop
    nop
    nop
    nop
    nop
    nop
1:
    jmp 1b
