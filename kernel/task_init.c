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

// 保证它们不跨64KB
u16 disk_buf1[256] __attribute__((__aligned__(512)));
u16 disk_buf2[256] __attribute__((__aligned__(512)));

void taskA_entry() {
    current->priority = 3;

    while (1) {
        sysc_wait(197);

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
    sysc_mkdir("/root/sbin/", 0777);

    {
        namei_t ni;
        const char *path = "/root/sbin/init.elf";
        // path_init(path, PATH_LOOKUP_PARENT, &ni);
        // path_walk(path, &ni);

        const int flags = O_CREAT | O_APPEND;

        path_open_namei(path, flags, S_IFREG, &ni);

        file_t file;
        file.f_flags = flags;
        file.f_ops = NULL;
        file.f_pos = 0;
        file.f_dentry = ni.path.dentry;
        file.f_ops = file.f_dentry->d_inode->i_fops;

        vfs_generic_file_write(&file, "aaa1234567", 10, &file.f_pos);

        file.f_pos = 0;
        char buf[128] = {'b', 'u', 'f'};
        vfs_generic_file_read(&file, buf, 4, &file.f_pos);
        for (int i = 0; i < 16; i++) {
            printk("%c ", buf[i]);
        }
        printk("\n");
    }
#endif
    void init_rootfs();
    init_rootfs();

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

#include <boot.h>
void init_rootfs() {
    void *mod_start = pa2va(boot_params.boot_module_begin);

    const uint32_t mod_magic = *(uint32_t *)(mod_start + 0);
    const uint32_t mod_head_size = *(uint32_t *)(mod_start + 4);
    const uint32_t mod_timestamp = *(uint32_t *)(mod_start + 8);
    const uint32_t mod_file_entry_cnt = *(uint32_t *)(mod_start + 12);
    const char *mod_name = (const char *)mod_start + 16;

    printk("%x %x\n", boot_params.boot_module_begin, boot_params.boot_module_end);
    printk("module magic %08x header size %u timestamp %u file entry cnt %u name %s \n", mod_magic, mod_head_size,
           mod_timestamp, mod_file_entry_cnt, mod_name);

    int file_entry_offset = mod_head_size;
    for (int i = 0; i < mod_file_entry_cnt; i++) {
        void *fe = mod_start + file_entry_offset;

        const uint32_t fe_size = *(uint32_t *)(fe + 0);
        const uint32_t fe_type = *(uint32_t *)(fe + 4);
        const uint32_t fe_filesz = *(uint32_t *)(fe + 8);
        const uint32_t fe_offset = *(uint32_t *)(fe + 12);
        const char *fe_name = (const char *)(fe + 16);

        file_entry_offset += fe_size;

        void *fc = mod_start + fe_offset;

        printk(">[fe:%u:%u] file size %u type %u name %s\n", i, fe_size, fe_filesz, fe_type, fe_name);

        for (int k = 0; k < 16; k++) {
            uint8_t c = *(uint8_t *)(fc + k);
            printk("%02X ", c);
        }
        printk("\n");

        {
            // TODO支持带多层目录的fe_name

            namei_t ni;
            const char *path = fe_name;
            const int flags = O_CREAT | O_APPEND;
#define bufsz 5223
            static char buf[bufsz] = {'b', 'u', 'f'};
#if 1
            int sysc_open(const char *path, int flags, int mode);
            int fd = sysc_open(path, flags, 0700);
            assert(fd >= 0);

            ssize_t sysc_write(int fd, const char *buf, size_t size);
            sysc_write(fd, fc, fe_filesz);

            ssize_t sysc_read(int fd, void *buf, size_t count);
            sysc_read(fd, buf, bufsz);
#else
            path_open_namei(path, flags, S_IFREG, &ni);

            file_t file;
            file.f_flags = flags;
            file.f_ops = NULL;
            file.f_pos = 0;
            file.f_dentry = ni.path.dentry;
            file.f_ops = file.f_dentry->d_inode->i_fops;

            vfs_generic_file_write(&file, fc, fe_filesz, &file.f_pos);

            file.f_pos = 0;

            vfs_generic_file_read(&file, buf, bufsz, &file.f_pos);
#endif
            for (int i = 0; i < bufsz; i++) {
                printk("%c", buf[i]);
            }
            printk("\n");
        }
    }
}
