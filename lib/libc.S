.extern main
.extern exit
.global _start

_start:
    nop
    nop
    nop
    nop
    nop
    nop

    pushl   $0xAAAAAAAA
    pushl   $0x22222222
    pushl   $0x11111111

    call main

    addl    $12, %esp

    pushl   $0
    call    exit
    addl    $4, %esp

    nop
    nop
    nop
    nop
    nop
    nop
    nop
die:
    jmp die
