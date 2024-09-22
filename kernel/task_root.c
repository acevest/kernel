/*
 * ------------------------------------------------------------------------
 *   File Name: task_root.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:20:22 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <disk.h>
#include <fcntl.h>
#include <ide.h>
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

int do_fork(pt_regs_t *regs, unsigned long flags);
int sysc_wait(int ticks);

#define get_eflags(x) __asm__ __volatile__("pushfl; popl %0;" : "=g"(x)::"memory")

void kernel_task(char *name, void *entry, void *arg) {
    pt_regs_t regs;

    memset((void *)&regs, 0, sizeof(regs));

    // 内核任务入口
    regs.edx = (unsigned long)entry;

    // 参数
    regs.ecx = (unsigned long)arg;

    regs.eax = (u32)name;

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
    get_eflags(regs.eflags);

    int pid = do_fork(&regs, FORK_KRNL);

    printd("kernel[%s] task pid is %d\n", name, pid);
}

// 测试用的代码
// 这里存的是下标对应的每个扇区的最后2个字节
// hexdump -C HD.IMG | less 看的，如果镜像文件有变动需要更新这里
uint16_t hd_sect_data_fingerprint[] = {
    0xAA55,  // 0
    0x0820,  // 1
    0x4d89,  // 2
    0xeb1d,  // 3
    0x1009   // 4
};

uint64_t debug_sect_nr = 0;

uint64_t get_next_deubug_sect_nr() {
    debug_sect_nr++;
#if 0
    debug_sect_nr %= sizeof(hd_sect_data_fingerprint) / sizeof(uint16_t);
#else
    if (debug_sect_nr >= sizeof(hd_sect_data_fingerprint) / sizeof(uint16_t)) {
        debug_sect_nr = 0;
    }
#endif
    return debug_sect_nr;
}

void verify_hd_data(uint64_t sect_nr, uint16_t *buf, const char *name) {
    uint16_t vfp = hd_sect_data_fingerprint[sect_nr];

    uint16_t fp = buf[255];

    if (fp == vfp) {
        // printk("%s verification passed sect %lu fp %04x\n", name, sect_nr, fp);
    } else {
        printk("%s verification failed sect %lu fp %04x right %04x\n", name, sect_nr, fp, vfp);
        panic("verify hd data fail");
    }
}

// 保存它们不跨64KB
u16 disk_buf1[256] __attribute__((__aligned__(512)));
u16 disk_buf2[256] __attribute__((__aligned__(512)));

void taskA_entry() {
    current->priority = 7;

    while (1) {
        sysc_wait(97);

        uint64_t sect_nr = get_next_deubug_sect_nr();
        memset(disk_buf1, 0, 512);

        disk_request_t r;
        r.dev = MAKE_DISK_DEV(0, 0);
        r.command = DISK_REQ_READ;
        r.pos = sect_nr;
        r.count = 1;
        r.buf = disk_buf1;
        r.bb = 0;

        send_disk_request(&r);

        // verify_hd_data(sect_nr, disk_buf1, current->name);

        for (int i = 0; i < 2; i++) {
            asm("hlt;");
        }
    }
}

void taskB_entry() {
    current->priority = 13;

    while (1) {
        sysc_wait(7);
        // uint64_t sect_nr = get_next_deubug_sect_nr();
        // memset(disk_buf2, 0, 512);
        // disk_request_t r;
        // r.dev = MAKE_DISK_DEV(2, 0);
        // r.command = DISK_REQ_READ;
        // r.pos = sect_nr;
        // r.count = 1;
        // r.buf = disk_buf2;
        // r.bb = 0;

        // send_disk_request(&r);
        // //  verify_hd_data(sect_nr, disk_buf2, current->name);

        // for (int i = 0; i < 1; i++) {
        //     asm("hlt;");
        // }
    }
}

void taskC_entry() {
    current->priority = 17;

    while (1) {
        sysc_wait(100);

        for (int i = 0; i < 7; i++) {
            asm("hlt;");
        }
    }
}

// 从multiboot.S进入这里
void root_task_entry() {
    sti();

    extern __attribute__((regparm(0))) long sysc_mkdir(const char *path, int mode);
    sysc_mkdir("/root", 0777);
    sysc_mkdir("/root/aaa", 0777);

    {
        namei_t ni;
        const char *path = "/root";
        path_init(path, 0, &ni);
        path_walk(path, &ni);
    }

    // 有一点点垃圾事情需要处理
    // 之前内核初始化都是在关中断下进行的
    // 这就段时间有可能按键盘，然而键盘不把数据读出来就不会触发下一次中断
    // 所以得先清空一下键盘
    inb(0x60);

    //
    void disk_init();
    disk_init();

    kernel_task("init", init_task_entry, NULL);
    kernel_task("ide/0", disk_task_entry, (void *)0);
    kernel_task("ide/1", disk_task_entry, (void *)1);
    kernel_task("user", user_task_entry, NULL);

    // for (int i = 0; i < 100; i++) {
    //     asm("hlt;");
    // }

#if 1
    kernel_task("tskA", taskA_entry, NULL);
    kernel_task("tskB", taskB_entry, NULL);
    kernel_task("tskC", taskC_entry, NULL);
#endif

    current->priority = 1;
    while (1) {
        asm("hlt;");
    }
}
