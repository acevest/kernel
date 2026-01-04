/*
 * ------------------------------------------------------------------------
 *   File Name: ap_boot.S
 *      Author: Zhao Yanbai
 *              2026-01-04 08:18:07 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

.code16
#.text
#.section .ap_boot, "ax", @progbits
.global ap_boot_bgn
.global ap_boot_end
.align 0x1000
ap_boot_bgn:
ap_realmode_base:
ap_real_mode_entry:
    cli
    # TODO 只允许CPU1运行

    wbinvd # 将CPU缓存写回主内存并使缓存失效

    xorl    %ebx, %ebx

    movw    %cs, %bx
    movw    %bx, %ds
    movw    %bx, %es
    movw    %bx, %ss

    # 算出ap_realmode_base的线性地址
    shl     $4, %ebx

    movl    %ebx, %esp
    addl    $0x1000, %esp

    # 算出gdt的线性地址
    leal    (ap_gdt - ap_realmode_base)(%ebx), %eax
    # 把这个地址填入gdtr的base位置
    movl    %eax, (ap_gdtr_base - ap_realmode_base)


    # 算出ap_code32_entry的线性地址
    leal    (ap_code32_entry - ap_realmode_base)(%ebx), %eax
    # 把这个地址填入ap_code32_entry_address
    mov     %eax, (ap_code32_entry_address - ap_realmode_base)


    lgdt    ap_gdtr - ap_realmode_base

    # enable PE
    movl    %cr0,%eax
    orl     $0x00000001,%eax
    movl    %eax,%cr0

    # 清空指令流水线
    jmp     .flush
.flush:

    # ljmpl 的前后两个l都很重要
    ljmpl    *(ap_code32_entry_address - ap_realmode_base)


.code32
.align 32
ap_code32_entry:
    cli
    nop
    nop

    # 数据段寄存器需要调整成GDT的描述符的选择子
    movl    $0x10, %eax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    %ax, %fs
    movw    %ax, %gs

    # 加载一个跳板页目录，为真正切换到内核地址空间做准备
    .extern ap_pre_pgd
    leal    ap_pre_pgd, %eax
    movl    %eax, %cr3

    # 开启分页
    movl    %cr0, %eax
    orl     $0x80000000, %eax
    movl    %eax, %cr0

    leal    ap_code32_real_entry, %eax
    jmp     *%eax

# 虽然下面的代码也会被复制到小于1MB的内存
# 但是它不会在那里被执行，而是会在大于1MB的物理内存，并且是内核的地址空间执行
# 也就是说他们在内核的原来的地址执行
# 当然也可以把它搬到ap_boot_end之后，这样它就不会被复制到小于1MB的内存里了
ap_code32_real_entry:

    nop
    nop

    # 把栈指针调整到内核的地址空间
    addl    $kernel_virtual_addr_start, %esp

    # 到这里我们就可以切到BSP一样cr3了
    # 为什么在开启分页前无法直接加载BSP的cr3?
    # 因为BSP把小于内核地址空间的映射都清0了，如果直接加载，分页后的第一行指令就无法执行

    .extern init_pgd
    leal    init_pgd, %eax
    subl    $kernel_virtual_addr_start, %eax # 算出物理地址
    movl    %eax, %cr3

    # 进入内核ap代码
    .extern ap_kernel_entry
    leal    ap_kernel_entry, %eax
    jmp    *%eax

    nop
    nop
    hlt


.align 32
ap_code32_entry_address:    .long 0x00000000 # 等待动态写入
                            .word 0x0008, 0x0000

.align 32
ap_gdt:
empty:     .long    0x00000000, 0x00000000
code_desc: .long    0x0000FFFF, 0x00CF9B00
data_desc: .long    0x0000FFFF, 0x00CF9300
ap_gdt_end:
ap_gdtr:
    ap_gdtr_limit: .word ap_gdt_end-ap_gdt
    ap_gdtr_base:  .long 0 # 等待动态写入

ap_boot_end:
