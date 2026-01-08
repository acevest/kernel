/*
 * ------------------------------------------------------------------------
 *   File Name: ap_boot.S
 *      Author: Zhao Yanbai
 *              2026-01-04 08:18:07 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

.code16
.global ap_boot_bgn
.global ap_boot_end
.align 0x1000
ap_boot_bgn:
ap_realmode_base:
ap_real_mode_entry:
    nop # 多AP启动时的锁的位置, 当然不加这个nop指令也行, 加在这里清晰一点

    cli

    wbinvd # 将CPU缓存写回主内存并使缓存失效

    xorl    %ebx, %ebx

    movw    %cs, %bx
    movw    %bx, %ds
    movw    %bx, %es
    movw    %bx, %ss

    xorb    %al, %al
    lock xchgb %al, %ds:0x00    # 就是最开始nop指令的位置
    cmpb    $0x00, %al
    jne     1f # 段内短跳

    # 如果没拿到锁, 则代表是第2个及以后的AP, 对这些AP直接hlt, 不再需要
    cli
2:
    nop
    nop
    #jmp 2b
    hlt

# 如果拿到锁, 则代表是第一个AP, 继续执行
1:
    # 算出ap_realmode_base的线性地址
    # 要把这个值传进保护模式
    shl     $4, %ebx

    # 将sp指向本页的末尾的位置
    # 这里不能写成 movl    %ebx, %esp; addl    $0x1000, %esp
    # 因为目前sp还是搭配ss来用的
    movl    $0x1000, %esp # ss:sp --> ss*16 + 0x1000
    #pushl   $0x12345678
    #popl    %eax

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


    # 算出保护模式的esp值
    addl    %ebx, %esp

    # 加载一个跳板页目录, 为真正切换到内核地址空间做准备
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
# 但是它不会在那里被执行, 而是会在大于1MB的物理内存(也就是在它原来的位置), 并且是内核的地址空间执行
# 也就是说他们在内核的原来的地址执行
# 当然也可以把它搬到ap_boot_end之后, 这样它就不会被复制到小于1MB的内存里了
ap_code32_real_entry:

    nop
    nop

    # 把栈指针调整到内核的地址空间
    addl    $kernel_virtual_addr_start, %esp

    # 到这里我们就可以切到BSP一样cr3了
    # 为什么在开启分页前无法直接加载BSP的cr3?
    # 因为BSP把小于内核地址空间的映射都清0了, 如果直接加载, 分页后的第一行指令就无法执行

    .extern init_pgd
    leal    init_pgd, %eax
    subl    $kernel_virtual_addr_start, %eax # 算出物理地址
    movl    %eax, %cr3

    # 加载AP的内核栈顶到esp
    .extern ap_get_kernel_esp
    call    ap_get_kernel_esp
    movl    %eax, %esp

    # 进入内核ap代码
    .extern ap_kernel_entry
    leal    ap_kernel_entry, %eax
    jmp    *%eax

    nop
    nop
    hlt


.align 32
.global ap_code32_entry_address
ap_code32_entry_address:    .long ap_code32_entry # 在BSP复制代码后会动态调整
                            .word 0x0008, 0x0000

# 下面的定义会一鱼两吃 [也会复制两份]:
#   1. 在实模式式下用一次 [只不过BSP复制到1MB以下的内存会稍作调整]
#   2. 在保护模式下用一次 [因为加载了最终的页目录就无法以一一映射的方式访问这段实模式内存]
.align 32
.global ap_gdtr
.global ap_gdtr_base
ap_gdtr:
    ap_gdtr_limit: .word ap_gdt_end-ap_gdt
    ap_gdtr_base:  .long ap_gdt # 在BSP复制代码后会动态调整


# 此处定义8个GDT项, 参考ap.c里的AP_GDT_CNT
.align 32
.global ap_gdt
ap_gdt:
empty:      .long    0x00000000, 0x00000000
ap_code:    .long    0x0000FFFF, 0x00CF9B00
ap_data:    .long    0x0000FFFF, 0x00CF9300
ap_tss:     .long    0x00000000, 0x00000000
pad0:       .long    0x00000000, 0x00000000
pad1:       .long    0x00000000, 0x00000000
pad2:       .long    0x00000000, 0x00000000
pad3:       .long    0x00000000, 0x00000000
ap_gdt_end:

ap_boot_end:
