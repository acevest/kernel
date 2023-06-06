/*
 * ------------------------------------------------------------------------
 *   File Name: task_user.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:21:31 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <fcntl.h>
#include <io.h>
#include <irq.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <stat.h>
#include <stdio.h>
#include <syscall.h>
#include <system.h>
#include <types.h>

#define __ring3text__ __attribute__((__section__(".ring3.text")))

int ring3_sysctest();
void __ring3text__ __attribute__((__aligned__(PAGE_SIZE))) ring3_entry() {
    while (1) {
        int __sysc_ret__ = 0;

        // 下面这段汇编中的 ring3_entry 和 sysexit_return_address的地址都是内核地址
        // 然而在用户态用的代码地址是从0x08000000开始的
        // 因此为了算了systexit返回的正确地址
        // 需要借助ring3_entry算出sysexit_return_address相对ring3_entry的偏移量
        // 再把这个偏移量加上0x08000000就是正确的sysexit的地址

        // 必需注意这里的sysexit_return_address并不是sysexit指令返回的地址
        // sysexit指令返回的是编译在内核中的一段代码地址，这个地址已经被设成内核和用户态共享
        // 在内核中的那段代码里会利用存在栈上的sysexit_return_address返回到
        // sysexit_return_address处
        asm volatile(
            "leal sysexit_return_address, %%ebx;"
            "subl %%ecx, %%ebx;"
            "addl $0x08000000, %%ebx;"  // 算出sysexit_return_address在用户态的实际地址
            "pushl %%ebx;"  // 把这个地址保存在栈上，内核sysexit会返回到一段共享代码上
                            // 共享代码会利用保存在栈上的地址返回到sysexit_return_address处
            "pushl $0;"
            "pushl $0;"
            "movl $2, %%ebx;"
            "pushl %%ebp;"
            "movl  %%esp, %%ebp;"
            "sysenter;"
            "sysexit_return_address:"
            : "=a"(__sysc_ret__)
            : "a"(SYSC_WAIT), "c"(ring3_entry));

        for (int i = 100000000; i > 0; i--) {
            for (int j = 1; j > 0; j--) {
                asm("nop;nop;nop;");
            }
        }
    }
}

void user_task_entry() {
    current->priority = 7;

    unsigned long ring3_text_page = va2pa(ring3_entry);
    unsigned long ring3_bss_page = va2pa(alloc_one_page(0));

    unsigned long *pt_text_page = (unsigned long *)(alloc_one_page(0));
    unsigned long *pt_bss_page = (unsigned long *)(alloc_one_page(0));

    unsigned long *p = (unsigned long *)(pa2va(current->cr3));

    printd("page dir : %x %x %x %x\n", p, pt_text_page, ring3_text_page);
    printd("pt bss page %x %x", pt_bss_page, ring3_bss_page);

    // text: 0x0800_0000
    //  bss: 0x3000_0000
    unsigned long text_at = 0x08000000;
    unsigned long bbs_at = 0x30000000;

    p[text_at >> 22] = (unsigned long)va2pa(pt_text_page) | PAGE_P | PAGE_US;
    pt_text_page[0] = ring3_text_page | PAGE_P | PAGE_US;
    p[bbs_at >> 22] = (unsigned long)va2pa(pt_bss_page) | PAGE_P | PAGE_WR | PAGE_US;
    pt_bss_page[0] = ring3_bss_page | PAGE_P | PAGE_WR | PAGE_US;

    //
    LoadCR3(current->cr3);

    // 现在要准备返回用户态
    // eip --> edx
    // esp --> ecx
    asm volatile("sysexit;" ::"d"(0x08000000), "c"(0x30000000 + PAGE_SIZE - 100));
}
