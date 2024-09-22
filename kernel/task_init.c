#include <disk.h>
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
int sysc_wait(int ticks);
void kernel_task(char *name, void *entry, void *arg);


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
        sysc_wait(7);

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

#if 1
        for (int i = 0; i < 2; i++) {
            asm("hlt;");
        }
#endif
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

#if 0
        // for (int i = 0; i < 1; i++) {
        //     asm("hlt;");
        // }
#endif
    }
}

void taskC_entry() {
    current->priority = 17;

    while (1) {
        sysc_wait(100);

#if 1
        for (int i = 0; i < 7; i++) {
            asm("hlt;");
        }
#endif
    }
}


void init_task_entry() {
    current->priority = 10;

//    pt_regs_t *child_regs = ((pt_regs_t *)(TASK_SIZE + (unsigned long)current)) - 1;
//    child_regs->eflags |= 0x200;
#if 1
    // 有一点点垃圾事情需要处理
    // 之前内核初始化都是在关中断下进行的
    // 这就段时间有可能按键盘，然而键盘不把数据读出来就不会触发下一次中断
    // 所以得先清空一下键盘
    inb(0x60);
#endif


#if 1
    extern __attribute__((regparm(0))) long sysc_mkdir(const char *path, int mode);
    sysc_mkdir("/root", 0777);
    sysc_mkdir("/root/aaa", 0777);

    {
        namei_t ni;
        const char *path = "/root";
        path_init(path, 0, &ni);
        path_walk(path, &ni);
    }
#endif

#if 1
    kernel_task("ide/0", disk_task_entry, (void *)0);

    void ide_read_partions();
    ide_read_partions();

    void ata_read_ext2_sb();
    ata_read_ext2_sb();
#endif

#if 0
    extern int ide_read_partions_done;
    while(!ide_read_partions_done) {
	    printk("wait for hard disk partition table done\n");
	    sysc_wait(1);
    }
#endif

#if 1
    kernel_task("ide/1", disk_task_entry, (void *)1);
    kernel_task("user", user_task_entry, NULL);
    kernel_task("tskA", taskA_entry, NULL);
    kernel_task("tskB", taskB_entry, NULL);
    kernel_task("tskC", taskC_entry, NULL);
#endif

    while (1) {
        sysc_wait(1);
    }
}
