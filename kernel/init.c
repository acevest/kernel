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

void root_task_entry();
int do_fork(pt_regs_t *regs, unsigned long flags);

System system;
TSS tss;
Desc idt[NIDT] __attribute__((__aligned__(8)));
Desc gdt[NGDT] __attribute__((__aligned__(8)));
char gdtr[6] __attribute__((__aligned__(4)));
char idtr[6] __attribute__((__aligned__(4)));

// char __initdata kernel_init_stack[KRNL_INIT_STACK_SIZE] __attribute__((__aligned__(PAGE_SIZE)));

// int debug_wait_queue_get();

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
            "pushl %%ebp;"
            "movl  %%esp, %%ebp;"
            "sysenter;"
            "sysexit_return_address:"
            : "=a"(__sysc_ret__)
            : "a"(SYSC_TEST), "c"(ring3_entry));
    }
}

void user_task_entry(char *name) {
    strcpy(current->name, name);

    current->priority = 90;

    unsigned long ring3_text_page = va2pa(ring3_entry);
    unsigned long ring3_bss_page = va2pa(alloc_one_page(0));

    unsigned long *pt_text_page = (unsigned long *)(alloc_one_page(0));
    unsigned long *pt_bss_page = (unsigned long *)(alloc_one_page(0));

    unsigned long *p = (unsigned long *)(pa2va(current->cr3));

    printk("page dir : %x %x %x %x\n", p, pt_text_page, ring3_text_page);
    printk("pt bss page %x %x", pt_bss_page, ring3_bss_page);

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

void init_task_entry(char *name) {
    int cnt = 0;
    pid_t id = sysc_getpid();

    strcpy(current->name, name);

    // 赋予不同的优先级
    current->priority = id * 30;

    if (current->priority <= 0) {
        current->priority = 1;
    }
    if (current->priority > 100) {
        current->priority = 100;
    }

    while (1) {
        // sysc_test();
        // printl(MPL_TASK_0 + id, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        // printl(MPL_TASK_1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        int v = 0;  // debug_wait_queue_get();
        // printk("task:%d wait queue get %d\n", id, v);
        asm("hlt;");
    }
}

void kernel_task(char *name, void *entry) {
    pt_regs_t regs;

    memset((void *)&regs, 0, sizeof(regs));

    // 内核任务入口
    regs.edx = (unsigned long)entry;

    // 创建内核任务的时候就直接指定其在fork后走的路径
    // 就不用走sysexit那个路径了
    extern void ret_from_fork_krnl();
    regs.eip = (unsigned long)ret_from_fork_krnl;
    regs.cs = SELECTOR_KRNL_CS;
    regs.ds = SELECTOR_KRNL_DS;
    regs.es = SELECTOR_KRNL_DS;
    regs.ss = SELECTOR_KRNL_DS;
    regs.fs = SELECTOR_KRNL_DS;
    regs.gs = SELECTOR_KRNL_DS;

    regs.ebx = (unsigned long)name;  // 用ebx传递参数

    int pid = do_fork(&regs, FORK_KRNL);

    printk("kernel[%s] task pid is %d\n", name, pid);
}

void root_task_entry() {
    sti();

    kernel_task("init", init_task_entry);
    kernel_task("test", init_task_entry);
    kernel_task("user", user_task_entry);

    int cnt = 0;

    while (1) {
        // sysc_test();
        // printl(MPL_TASK_0, "root:0 [%08x] weight %d cnt %d", current, root_task.weight, cnt++);
        // printk("root:0 [%08x] weight %d cnt %d", current, current->weight, cnt++);
        asm("sti;hlt;");
        // asm("nop;nop;nop;");
        // sysc_test();
        // syscall0(SYSC_TEST);
    }
}
