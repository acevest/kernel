# 仅用来调试

.global _start
.section .text
_start:
    nop;
    nop;
    nop;
    #leal 1f,%eax;
    #pushl %eax
    pushl $1f; # 这里push 1f和$1f意义不一样
    pushl %ecx;
    pushl %edx;
    pushl %ebp;
    nop;
    nop;
    movl %esp,%ebp;
    nop;
    movl $123,%ebx
    movl $5,%eax # SYSC_WAIT
    sysenter;
1:
    nop;
    nop;
    nop;
    movl $300000000, %ecx
2:
    nop
    nop
    nop
    nop
    nop
    loop 2b
    jmp _start
