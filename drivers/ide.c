/*
 * ------------------------------------------------------------------------
 *   File Name: ide.c
 *      Author: Zhao Yanbai
 *              Sat May 24 16:30:38 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

// #include <assert.h>
#include <ide.h>
// #include <io.h>
#include <irq.h>

// #include <printk.h>
#include <semaphore.h>
#include <task.h>
// #include <string.h>
// #include <types.h>
#include <wait.h>

// typedef struct prd {
//     unsigned int addr;
//     unsigned int cnt : 16;
//     unsigned int reserved : 15;
//     unsigned int eot : 1;
// } prd_t;

// typedef struct {
//     u64_t lba;
//     u32_t scnt;
//     u32_t read_scnt;
//     char *buf;
//     bool finish;
//     wait_queue_head_t wait;
// } ide_request_t;

typedef void (*ide_intr_func_t)();

void ide_default_intr();

// ide_drive_t drv;
// DECLARE_MUTEX(mutex);
// ide_request_t ide_request;

ide_intr_func_t ide_intr_func = ide_default_intr;

// unsigned char *dma_data = 0;

// unsigned int HD_CHL0_CMD_BASE = 0x1F0;
// unsigned int HD_CHL1_CMD_BASE = 0x170;

// unsigned int HD_CHL0_CTL_BASE = 0x3F6;
// unsigned int HD_CHL1_CTL_BASE = 0x376;

// void ide_printl() { printl(MPL_IDE, "ide pio cnt %d dma cnt %d irq cnt %d", drv.pio_cnt, drv.dma_cnt, drv.irq_cnt); }

// void ide_cmd_out(dev_t dev, u32 sect_cnt, u64 sect_nr, u32 cmd) {
//     drv.pio_cnt++;
//     drv.read_mode = cmd;

//     ide_printl();

//     outb(0x00, REG_CTL(dev));
//     outb(0x40 | 0x00, REG_DEVICE(dev));

//     outb((u8)((sect_cnt >> 8) & 0xFF), REG_NSECTOR(dev));  // High
//     outb((u8)((sect_nr >> 24) & 0xFF), REG_LBAL(dev));
//     outb((u8)((sect_nr >> 32) & 0xFF), REG_LBAM(dev));
//     outb((u8)((sect_nr >> 40) & 0xFF), REG_LBAH(dev));

//     outb((u8)((sect_cnt >> 0) & 0xFF), REG_NSECTOR(dev));  // Low
//     outb((u8)((sect_nr >> 0) & 0xFF), REG_LBAL(dev));
//     outb((u8)((sect_nr >> 8) & 0xFF), REG_LBAM(dev));
//     outb((u8)((sect_nr >> 16) & 0xFF), REG_LBAH(dev));

//     outb(cmd, REG_CMD(dev));
// }

// part_t *ide_get_part(dev_t dev) {
//     assert(DEV_MAJOR(dev) == DEV_MAJOR_HDA);
//     assert(DEV_MINOR(dev) < MAX_SUPPORT_PARTITION_CNT);

//     return drv.part + DEV_MINOR(dev);
// }

// void ide_do_read(u64_t lba, u32_t scnt, char *buf) {
//     bool finish = false;
//     unsigned long flags;

//     ide_request_t *r = &ide_request;

//     down(&mutex);

//     r->lba = lba;
//     r->scnt = scnt;
//     r->read_scnt = 0;
//     r->buf = buf;
//     r->finish = false;
//     init_wait_queue(&r->wait);

//     task_union *task = current;
//     DECLARE_WAIT_QUEUE(wait, task);
//     add_wait_queue(&r->wait, &wait);

//     ide_cmd_out(0, scnt, lba, HD_CMD_READ_PIO_EXT);

//     while (true) {
//         // printd("%s pid %d is going to wait\n", __func__, sysc_getpid());
//         task->state = TASK_WAIT;
//         irq_save(flags);
//         finish = r->finish;
//         // printd("%s pid %d finish %u read_scnt %u scnt %u\n", __func__, sysc_getpid(), r->finish, r->read_scnt,
//         // r->scnt);
//         irq_restore(flags);

//         if (finish) break;

//         schedule();
//         // printd("%s pid %d is running\n", __func__, sysc_getpid());
//     }

//     // printd("%s pid %d is really running\n", __func__, sysc_getpid());
//     task->state = TASK_READY;
//     del_wait_queue(&r->wait, &wait);
// }

// unsigned int sys_clock();

ide_pci_controller_t ide_pci_controller;

extern unsigned int ATA_CHL0_CMD_BASE;
extern unsigned int ATA_CHL1_CMD_BASE;

extern unsigned int ATA_CHL0_CTL_BASE;
extern unsigned int ATA_CHL1_CTL_BASE;

void ide_pci_init(pci_device_t *pci) {
    unsigned int v;

    v = pci_read_config_word(pci_cmd(pci, PCI_COMMAND));
    // printk(" ide pci command %04x\n", v);

    v = pci_read_config_byte(pci_cmd(pci, PCI_PROGIF));
    printk(" ide pci program interface %02x\n", v);

    unsigned int iobase = pci_read_config_long(pci_cmd(pci, PCI_BAR4));
    printk(" ide pci Base IO Address Register %08x\n", iobase);
    iobase &= 0xFFFC;  // 最低为0是内存地址为1是端口地址
    ide_pci_controller.bus_iobase = iobase;
    ide_pci_controller.bus_cmd = iobase + PCI_IDE_CMD;
    ide_pci_controller.bus_status = iobase + PCI_IDE_STATUS;
    ide_pci_controller.bus_prdt = iobase + PCI_IDE_PRDT;
    ide_pci_controller.prdt = (prdte_t *)alloc_one_page(0);

    int i;
    printk(" BARS: ");
    for (i = 0; i < BARS_CNT; ++i) {
        printk("%08x ", pci->bars[i]);
        pci->bars[i] &= (~1UL);
    }
    printk("\n");

    ATA_CHL0_CMD_BASE = pci->bars[0] ? pci->bars[0] : ATA_CHL0_CMD_BASE;
    ATA_CHL0_CTL_BASE = pci->bars[1] ? pci->bars[1] : ATA_CHL0_CTL_BASE;

    ATA_CHL1_CMD_BASE = pci->bars[2] ? pci->bars[2] : ATA_CHL1_CMD_BASE;
    ATA_CHL1_CTL_BASE = pci->bars[3] ? pci->bars[3] : ATA_CHL1_CTL_BASE;

    ide_pci_controller.pci = pci;

    printd("channel0: cmd %04x ctl %04x channel1: cmd %04x ctl %04x\n", ATA_CHL0_CMD_BASE, ATA_CHL0_CTL_BASE,
           ATA_CHL1_CMD_BASE, ATA_CHL1_CTL_BASE);
    // printl(18, "channel0: cmd %04x ctl %04x channel1: cmd %04x ctl %04x", HD_CHL0_CMD_BASE, HD_CHL0_CTL_BASE,
    // HD_CHL1_CMD_BASE, HD_CHL1_CTL_BASE);
}

// void ide_status() {
//     u8_t idest = inb(REG_STATUS(0));
//     u8_t pcist = inb(drv.bus_status);
//     printk(" ide status %02x pci status %02x\n", idest, pcist);
// }

// void ide_debug() {
//     unsigned int nsect = 1;
//     char *buf = kmalloc(1 * SECT_SIZE, 0);
//     if (buf == 0) panic("out of memory");

//     ide_do_read(0, nsect, buf);

//     u16_t sig = *((u16_t *)(buf + 510));
//     printk("%s SIG: %04x\n", __func__, sig);

//     kfree(buf);
// }
const char *pci_get_info(unsigned int classcode, unsigned int progif);
void init_pci_controller(unsigned int classcode) {
    pci_device_t *pci = pci_find_device_by_classcode(classcode);
    if (pci != 0 && pci->intr_line < 16) {
        printk("found pci %03d:%02d.%d #%02d %04X:%04X ProgIF %02x %s\n", pci->bus, pci->dev, pci->devfn,
               pci->intr_line, pci->vendor, pci->device, pci->progif, pci_get_info(pci->classcode, pci->progif));
        // printk("found pci vendor %04x device %04x class %04x intr %d progif: %x\n", pci->vendor, pci->device,
        //        pci->classcode, pci->intr_line, pci->progif);
        // printl(17, "found pci vendor %04x device %04x class %04x intr %d", pci->vendor, pci->device,
        // pci->classcode,
        // pci->intr_line);
        ide_pci_init(pci);
        // while (1) asm("cli;hlt;");
    }
}

void ide_default_intr() {}
// void ide_default_intr() {
//     // printd("%s\n", __func__);
//     u8_t status = inb(REG_STATUS(0));

//     drv.irq_cnt++;

//     status = inb(drv.bus_status);
//     if (0 == (status & PCI_IDE_STATUS_INTR)) {
//         return;
//     }

//     status |= PCI_IDE_STATUS_INTR;
//     outb(status, drv.bus_status);
//     outb(0x00, drv.bus_cmd);

//     u16_t sig = 0;
//     if (drv.read_mode == HD_CMD_READ_PIO_EXT) {
//         insl(REG_DATA(0), ide_request.buf + ide_request.read_scnt * (SECT_SIZE), (SECT_SIZE) >> 2);
//         ide_request.read_scnt++;
//         sig = *((u16_t *)(ide_request.buf + 510));
//     }

//     if (drv.read_mode == HD_CMD_READ_DMA_EXT) {
//         sig = *((u16_t *)(dma_data + 510));
//     }

//     ide_printl();

//     // printd(" hard disk sig %04x read mode %x cnt %d\n", sig, drv.read_mode, drv.irq_cnt);
//     printl(MPL_IDE_INTR, "hard disk sig %x read mode %x cnt %d", sig, drv.read_mode, drv.irq_cnt);

//     outb(PCI_IDE_CMD_STOP, drv.bus_cmd);

//     wake_up(&ide_request.wait);
//     if (drv.read_mode == HD_CMD_READ_PIO_EXT) {
//         if (ide_request.read_scnt == ide_request.scnt) ide_request.finish = true;
//     }

//     up(&mutex);
// }

void ide_irq() { ide_intr_func(); }

// prd_t prd __attribute__((aligned(64 * 1024)));
// unsigned long gprdt = 0;

// #define DELAY400NS             \
//     {                          \
//         inb(HD_CHL0_CTL_BASE); \
//         inb(HD_CHL0_CTL_BASE); \
//         inb(HD_CHL0_CTL_BASE); \
//         inb(HD_CHL0_CTL_BASE); \
//     }

// void ide_dma_pci_lba48() {
//     drv.dma_cnt++;
//     drv.read_mode = HD_CMD_READ_DMA_EXT;
// #if 1
//     memset((void *)&prd, 0, sizeof(prd));
//     unsigned long addr = alloc_one_page(0);
//     dma_data = (char *)addr;
//     memset(dma_data, 0xBB, 512);
//     prd.addr = va2pa(addr);
//     prd.cnt = 512;
//     prd.eot = 1;
//     gprdt = va2pa(&prd);

//     printl(16, "gprdt %08x &prdt %08x prd.addr %08x addr %08x", gprdt, &prd, prd.addr, addr);

//     outb(PCI_IDE_CMD_STOP, drv.bus_cmd);
//     unsigned short status = inb(drv.bus_status);
//     outb(status | PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, drv.bus_status);
//     outl(gprdt, drv.bus_prdt);
//     outb(PCI_IDE_CMD_WRITE, drv.bus_cmd);
// #endif

// #if 0
//     while ( 1 )
//     {
//         status = inb(HD_CHL0_CMD_BASE+HD_STATUS);
//         printk(" <%02x> ", status);
//         if((status & (HD_STATUS_BSY | HD_STATUS_DRQ)) == 0)
//         {
//             break;
//         }
//     }

//     outb(0x00, HD_CHL0_CMD_BASE+HD_DEVICE);
//     DELAY400NS;

//     while ( 1 )
//     {
//         status = inb(HD_CHL0_CMD_BASE+HD_STATUS);
//         printk(" <%02x> ", status);
//         if((status & (HD_STATUS_BSY | HD_STATUS_DRQ)) == 0)
//         {
//             break;
//         }
//     }
// #endif

//     outb(0x00, HD_CHL0_CTL_BASE);  // Device Control

//     outb(0x00, HD_CHL0_CMD_BASE + HD_FEATURES);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_NSECTOR);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_LBAL);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_LBAM);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_LBAH);

//     outb(0x00, HD_CHL0_CMD_BASE + HD_FEATURES);
//     outb(0x01, HD_CHL0_CMD_BASE + HD_NSECTOR);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_LBAL);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_LBAM);
//     outb(0x00, HD_CHL0_CMD_BASE + HD_LBAH);

//     outb(0x40, HD_CHL0_CMD_BASE + HD_DEVICE);

//     outb(HD_CMD_READ_DMA_EXT, HD_CHL0_CMD_BASE + HD_CMD);

//     inb(drv.bus_cmd);
//     inb(drv.bus_status);
//     unsigned short w = inb(drv.bus_cmd);
//     outb(w | PCI_IDE_CMD_WRITE | PCI_IDE_CMD_START, drv.bus_cmd);
//     inb(drv.bus_cmd);
//     inb(drv.bus_status);
// }

// typedef struct {
//     u8_t a;
//     u8_t b;
//     u16_t lbah;  // lba high
//     u8_t type;
//     u8_t f;
//     u16_t scnth;  // sector count high
//     u32_t lba;    // lba low
//     u32_t scnt;   // sector count
// } hd_part_t;

// void ide_read_extended_partition(u64_t lba, unsigned int inx) {
//     if (inx >= MAX_SUPPORT_PARTITION_CNT) return;

//     unsigned int i;
//     char *buf = kmalloc(512, 0);
//     if (buf == 0) panic("no memory");

//     ide_do_read(lba, 1, buf);

//     u16_t sig = *((u16_t *)(buf + 510));
//     if (sig != 0xAA55) panic("bad partition sect");

//     hd_part_t *p = (hd_part_t *)(buf + PARTITION_TABLE_OFFSET);
//     printd("%s:%d lba %d \n", __func__, __LINE__, lba);

//     for (i = 0; i < PARTITION_CNT; ++i, ++p) {
//         if (p->type == 0) continue;

//         // u64_t   part_lba = lba + (p->lba|((p->lbah*1ULL)<<32));
//         // u64_t   part_scnt= p->scnt | ((p->scnth*1ULL)<<32);
//         u64_t part_lba = lba + p->lba;
//         u64_t part_scnt = p->scnt;

//         if (p->type != 0x05) {
//             drv.part[inx].lba_start = part_lba;
//             drv.part[inx].lba_end = part_lba + part_scnt;
//             printk("  logic partition[%02d] [%02x] LBA base %10d end %10d\n", inx, p->type,
//                    (unsigned int)(drv.part[inx].lba_start), (unsigned int)(drv.part[inx].lba_end - 1));
//         } else {
//             part_lba = drv.ext_lba_base + p->lba;
//             printk("        extended      [%02x] LBA base %10d end %10d\n", p->type, (unsigned int)(part_lba),
//                    (unsigned int)(part_lba + part_scnt - 1));
//             ide_read_extended_partition(part_lba, inx + 1);
//         }
//     }

//     kfree(buf);
// }

// void ide_read_partition() {
//     printk("reading partitions....\n");
//     unsigned int i;
//     char *buf = kmalloc(512, 0);
//     if (buf == 0) panic("no memory");

//     ide_do_read(0, 1, buf);

//     u16_t sig = *((u16_t *)(buf + 510));
//     if (sig != 0xAA55) panic("bad partition sect");

//     hd_part_t *p = (hd_part_t *)(buf + PARTITION_TABLE_OFFSET);

//     unsigned int ext_inx = ~0U;

//     for (i = 0; i < PARTITION_CNT; ++i, ++p) {
//         if (p->type == 0) continue;

//         // u64_t   part_lba = p->lba|((p->lbah*1ULL)<<32);
//         // u64_t   part_scnt= p->scnt | ((p->scnth*1ULL)<<32);
//         u64_t part_lba = p->lba;
//         u64_t part_scnt = p->scnt;

//         drv.part[i].lba_start = part_lba;
//         drv.part[i].lba_end = part_lba + part_scnt;

//         if (p->type == 0x05) {
//             if (drv.ext_lba_base == 0) {
//                 drv.ext_lba_base = drv.part[i].lba_start;
//                 ext_inx = i;
//             }
//         }

//         printk("primary partition[%02d] [%02x] LBA base %10d end %10d\n", i, p->type, (unsigned int)(part_lba),
//                (unsigned int)(part_lba + part_scnt - 1));
//     }

//     kfree(buf);

//     if (ext_inx != ~0U) ide_read_extended_partition(drv.part[ext_inx].lba_start, 4);
// }

// u16 ide_identify_buf[256];

// static void ata_io_wait() {
//     for (int i = 0; i < 128; i++) {
//         inb(REG_CTL(0));
//     }
// }

// // https://wiki.osdev.org/ATA_PIO_Mode
// // To use the IDENTIFY command, select a target drive by sending 0xA0 for the master drive, or 0xB0 for the
// slave,
// // to the "drive select" IO port. On the Primary bus, this would be port 0x1F6. Then set the Sectorcount, LBAlo,
// // LBAmid, and LBAhi IO ports to 0 (port 0x1F2 to 0x1F5). Then send the IDENTIFY command (0xEC) to the Command IO
// // port (0x1F7).
// // Then read the Status port (0x1F7) again. If the value read is 0, the drive does not exist. For any
// // other value: poll the Status port (0x1F7) until bit 7 (BSY, value = 0x80) clears. Because of some ATAPI drives
// // that do not follow spec, at this point you need to check the LBAmid and LBAhi ports (0x1F4 and 0x1F5) to see
// if
// // they are non-zero. If so, the drive is not ATA, and you should stop polling. Otherwise, continue polling one
// of
// // the Status ports until bit 3 (DRQ, value = 8) sets, or until bit 0 (ERR, value = 1) sets. At that point, if
// ERR
// // is clear, the data is ready to read from the Data port (0x1F0). Read 256 16-bit values, and store them.
// //
// // ATAPI的情况暂时不用考虑，因为不是硬盘相关的
// // https://wiki.osdev.org/ATAPI
// // ATAPI refers to devices that use the Packet Interface of the ATA6 (or higher) standard command set. It is
// // basically a way to issue SCSI commands to a CD-ROM, CD-RW, DVD, or tape drive, attached to the ATA bus.
// //
// // 总结来说，仅考虑ATA硬盘的情况
// // 一个IDE接口能接Master、Slave两个DRIVE。
// // 一个PC机上通常有两个IDE接口(IDE0, IDE1或ATA0, ATA1)，通常称通道0、1
// //
// 对于同一个IDE通道的两个DRIVE，共享同一组寄存器，它们之间的区分是通过Device寄存器的第4个bit位来实现的。0为Master，1为Slave
// //
// // 使用IDENTIFY命令步骤:
// //  1. 选择DRIVE，发送0xA0选择master，发送0xB0选择slave。(发送 0xE0 | (drive << 4)到Device寄存器)
// //  2. 发送0到该DRIVE所在通道的寄存器NSECTOR, LBAL, LBAM, LBAH
// //  3. 发送IDENTIFY(0xEC)命令到该通道的命令寄存器
// // 检查status寄存器：
// //  1. 若为0，就认为没有IDE
// //  2. 等到status的BSY位清除
// //  3. 等到status的DRQ位或ERR位设置
// void ide_read_identify() {
//     int dev = 0;  // 这里所用的dev是逻辑编号 ATA0、ATA1下的Master、Salve的dev分别为0,1,2,3
//     // outb(0xE0 | ((dev & 0x01) << 4), REG_DEVICE(dev));  // 这里可也可以发送(0xA0 | ((dev & 0x0ls1) << 4))
//     outb(0x00 | ((dev & 0x01) << 4), REG_DEVICE(dev));  // 这里可也可以发送(0xA0 | ((dev & 0x01) << 4))
//     outb(0x00, REG_NSECTOR(0));
//     outb(0x00, REG_LBAL(0));
//     outb(0x00, REG_LBAM(0));
//     outb(0x00, REG_LBAH(0));
//     outb(HD_CMD_IDENTIFY, REG_CMD(dev));
//     while (true) {
//         u8_t status = inb(REG_STATUS(0));
//         if (status == 0) {
//             panic("no hard drive");
//         }
//         u8_t error = inb(REG_ERR(0));
//         printk("hd0 status: %x %x %x\n", status, error, REG_STATUS(0));
//         if ((status & HD_STATUS_BSY) == 0 && (status & HD_STATUS_DRQ) != 0) {
//             break;
//         }
//     }

//     insw(REG_DATA(0), ide_identify_buf, 256);

//     // 第83个word(2bytes)的第10个bit位表示是否支持LBA48，为1表示支持。

//     for (int i = 0; i < 256; i++) {
//         if (i % 12 == 0) {
//             printk("\n%d ", i);
//         }
//         printk("%04x ", ide_identify_buf[i]);
//     }

//     struct iden_info {
//         int idx;
//         int len;
//         char *desc;
//     } infos[] = {{10, 20, "HD SN"}, {17, 8, "Firmware Revision"}, {27, 40, "HD Model"}};

//     u16 *hdinfo = (u16 *)ide_identify_buf;
//     char s[64];
//     for (int i = 0; i < sizeof(infos) / sizeof(infos[0]); i++) {
//         char *p = (char *)&hdinfo[infos[i].idx];
//         int j = 0;
//         for (j = 0; j < infos[i].len / 2; j++) {
//             s[j * 2 + 1] = *p++;
//             s[j * 2] = *p++;
//         }
//         s[j] = 0;
//         printk("%s: %s\n", infos[i].desc, s);
//     }
// }

// void ide_detect() {
//     // outb(HD_CTL_SRST, REG_CTL(0));
//     outb(0xA0 | 0x00, REG_DEVICE(dev));
//     ata_io_wait();

//     unsigned cl = inb(REG_LBAM(0));
//     ata_io_wait();
//     unsigned ch = inb(REG_LBAH(0));
//     ata_io_wait();
//     printk("%02x %02x\n", cl, ch);
// }

// void ata_read_identify(int dev);

#if 1
extern semaphore_t disk_intr_sem;
#else
DECLARE_WAIT_QUEUE_HEAD(ide_wait_queue_head);

void sleep_on_ide() { sleep_on(&ide_wait_queue_head); }

void prepare_to_wait_on_ide() { ide_pci_controller.done = 0; }
void wait_on_ide() { wait_event(&ide_wait_queue_head, ide_pci_controller.done); }
#endif

extern void *mbr_buf;
extern ide_pci_controller_t ide_pci_controller;
extern uint32_t disk_request_cnt;
extern uint32_t disk_handled_cnt;

uint8_t ata_pci_bus_status();

volatile uint32_t disk_inter_cnt = 0;

void ide_irq_bh_handler() {
    disk_inter_cnt++;

    // printl(MPL_IDE, "disk req %u consumed %u irq %u", disk_request_cnt, disk_handled_cnt, disk_inter_cnt);
    printlxy(MPL_IDE, MPO_IDE, "disk irq %u req %u consumed %u ", disk_inter_cnt, disk_request_cnt, disk_handled_cnt);

    // up里不会立即重新调度进程
    up(&disk_intr_sem);
}

void ide_irq_handler(unsigned int irq, pt_regs_t *regs, void *devid) {
    // printk("ide irq %d handler pci status: 0x%02x\n", irq, ata_pci_bus_status());

    add_irq_bh_handler(ide_irq_bh_handler);
}

void ide_init() {
    // memset((void *)&drv, 0, sizeof(drv));
    memset(&ide_pci_controller, 0, sizeof(ide_pci_controller));

    void ide_ata_init();
    ide_ata_init();

    request_irq(0x0E, ide_irq_handler, "hard", "IDE");

    // init_pci_controller(0x0106);
    init_pci_controller(0x0101);
}
