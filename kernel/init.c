#include <fcntl.h>
#include <init.h>
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

System system;
TSS tss;
Desc idt[NIDT] __attribute__((__aligned__(8)));
Desc gdt[NGDT] __attribute__((__aligned__(8)));
char gdtr[6] __attribute__((__aligned__(4)));
char idtr[6] __attribute__((__aligned__(4)));

// char __initdata kernel_init_stack[KRNL_INIT_STACK_SIZE] __attribute__((__aligned__(PAGE_SIZE)));

// int debug_wait_queue_get();

#define __ring3text__ __attribute__((__section__(".ring3.text")))
#define __ring3data__ __attribute__((__section__(".ring3.data"), __aligned__(PAGE_SIZE)))
#define __ring3bss__ __attribute__((__section__(".ring3.bss"), __aligned__(PAGE_SIZE)))

char __ring3data__ ring3_stack[PAGE_SIZE] = {0};
char __ring3bss__ ring3_stack[PAGE_SIZE];
int ring3_sysctest();
void __ring3text__ __attribute__((__aligned__(PAGE_SIZE))) ring3_entry() {
    while (1) {
        ring3_sysctest();
    }
}


static int __ring3text__ __volatile__ __ring3_syscall0(int nr) {
    int __sysc_ret__ = 0;
    extern void sysenter;
    asm volatile("leal sysenter, %%ebx;call *%%ebx;" : "=a"(__sysc_ret__) : "a"(nr));
    return __sysc_ret__;
}
int __ring3text__ _ring3_syscall0(int nr) { return __ring3_syscall0(nr); }
int __ring3text__ ring3_sysctest() { return _ring3_syscall0(SYSC_TEST); }

void user_task_entry() {
    // printk("user_task_entry: %08x\n", ring3_entry);

    // unsigned long ring3_text_page = va2pa(alloc_one_page(0));
    // unsigned long ring3_data_page = va2pa(alloc_one_page(0));
    // unsigned long ring3_bss_page = va2pa(alloc_one_page(0));

    unsigned long ring3_text_page = va2pa(ring3_entry);
    unsigned long ring3_data_page = va2pa(ring3_stack);
    unsigned long ring3_bss_page = va2pa(alloc_one_page(0));

    unsigned long *pt_text_page = (unsigned long *)(alloc_one_page(0));
    unsigned long *pt_data_page = (unsigned long *)(alloc_one_page(0));
    unsigned long *pt_bss_page = (unsigned long *)(alloc_one_page(0));
    unsigned long *p = (unsigned long *)(pa2va(current->cr3));

    //asm volatile("xchg %%bx, %%bx;mov %%eax, %%ebx;xchg %%bx, %%bx;"::"a"(p));
    printk("page dir : %x %x %x %x\n", p, pt_text_page, ring3_text_page);
    printk("pt bss page %x %x", pt_bss_page, ring3_bss_page);

    // text: 0x0800_0000
    // data: 0x2000_0000
    //  bss: 0x3000_0000
    unsigned long text_at = 0x08000000;
    unsigned long data_at = 0x20000000;
    unsigned long bbs_at = 0x30000000;

    p[text_at >> 22] = (unsigned long)va2pa(pt_text_page) | PAGE_P | PAGE_WR | PAGE_US;
    pt_text_page[0] = ring3_text_page | 7;
    p[data_at >> 22] = (unsigned long)va2pa(pt_data_page) | PAGE_P | PAGE_WR | PAGE_US;
    pt_data_page[0] = ring3_data_page | 7 ;
    p[bbs_at >> 22] = (unsigned long)va2pa(pt_bss_page) | PAGE_P | PAGE_WR | PAGE_US;
    pt_bss_page[0] = ring3_bss_page | 7 ;

    // 
    LoadCR3(current->cr3);

    // 现在要准备返回用户态
    // eip --> edx
    // esp --> ecx
   asm volatile("xchg %bx, %bx");
   asm volatile("sysexit;" ::"d"(0x08000000), "c"(0x30000000 + PAGE_SIZE - 100));
}

void init_task_entry() {
    int cnt = 0;
    pid_t id = sysc_getpid();

    // 赋予不同的优先级
    current->priority = id * 30;
    if (current->priority <= 0) {
        current->priority = 1;
    }
    if (current->priority > 100) {
        current->priority = 100;
    }

    while (1) {
        //sysc_test();
        printl(MPL_TASK_1 + id - 1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
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
        //sysc_test();
        printl(MPL_ROOT, "root:0 [%08x] weight %d cnt %d", current, root_task.weight, cnt++);
        // printk("root:0 [%08x] weight %d cnt %d", current, current->weight, cnt++);
        asm("sti;hlt;");
        // asm("nop;nop;nop;");
        //sysc_test();
        // syscall0(SYSC_TEST);
    }
}
