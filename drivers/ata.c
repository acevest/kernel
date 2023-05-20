/*
 * ------------------------------------------------------------------------
 *   File Name: ata.c
 *      Author: Zhao Yanbai
 *              2021-11-10 13:55:27 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <ata.h>
#include <disk.h>
#include <ide.h>
#include <io.h>
#include <irq.h>
#include <sched.h>
#include <string.h>
#include <system.h>

extern ide_pci_controller_t ide_pci_controller;

typedef struct _ide_drive {
    int present;
    int dma;
    uint64_t lba48;
    uint64_t max_lba;
} ide_drive_t;

#define MAX_IDE_DRIVE_CNT 4
ide_drive_t ide_drives[MAX_IDE_DRIVE_CNT];

#define ATA_TIMEOUT 10  // 10次时钟中断

void ata_dma_read_ext(int dev, uint64_t pos, uint16_t count, void *dest);
int ata_pio_read_ext(int dev, uint64_t pos, uint16_t count, int timeout, void *dest);

void *mbr_buf;
void ata_test(uint64_t nr) {
    memset(mbr_buf, 0xAA, SECT_SIZE);
    ata_dma_read_ext(0, nr, 1, mbr_buf);
}

// 本程序参考文档《AT Attachment with Packet Interface - 6》

// 仅考虑ATA硬盘的情况
// 一个IDE接口能接Master、Slave两个DRIVE。
// 一个PC机上通常有两个IDE接口(IDE0, IDE1或ATA0, ATA1)，通常称通道0、1
// 对于同一个IDE通道的两个DRIVE，共享同一组寄存器，它们之间的区分是通过Device寄存器的第4个bit位来实现的。0为Master，1为Slave
//
// 使用IDENTIFY命令步骤:
//  1. 选择DRIVE构造命令，发送到Device寄存器(选择master发送: 0x00, 选择slave发送: 0x40)
//  2. 发送IDENTIFY(0xEC)命令到该通道的命令寄存器
// 检查status寄存器：
//  1. 若为0，就认为没有IDE
//  2. 等到status的BSY位清除
//  3. 等到status的DRQ位或ERR位设置
u16 identify[256];
void ata_send_read_identify_cmd(int dev) {}

void ata_read_data(int dev, int sect_cnt, void *dst) { insl(REG_DATA(dev), dst, (512 * sect_cnt) / sizeof(uint32_t)); }

void ata_read_identify(int dev, int disable_intr) {
    uint8_t ctlv = 0x00;
    if (disable_intr != 0) {
        ctlv |= ATA_CTL_NIEN;
    }
    outb(ctlv, REG_CTL(dev));
    outb(0x00 | ((dev & 0x01) << 4), REG_DEVICE(dev));  // 根据文档P113，这里不用指定bit5, bit7，直接指示DRIVE就行
    outb(ATA_CMD_IDENTIFY, REG_CMD(dev));
}

void ide_ata_init() {
    for (int i = 0; i < MAX_IDE_DRIVE_CNT; i++) {
        int dev = i;

        ata_read_identify(dev, 1);

        uint8_t status = inb(REG_STATUS(dev));
        if (status == 0 || (status & ATA_STATUS_ERR) || (status & ATA_STATUS_RDY == 0)) {
            ide_drives[i].present = 0;
            continue;
        } else {
            ide_drives[i].present = 1;
        }

        printk("ata[%d] status %x %s exists\n", i, status, ide_drives[i].present == 1 ? "" : "not");
        insl(REG_DATA(dev), identify, SECT_SIZE / sizeof(uint32_t));

        // 第49个word的第8个bit位表示是否支持DMA
        // 第83个word的第10个bit位表示是否支持LBA48，为1表示支持。
        // 第100~103个word的八个字节表示user的LBA最大值
        // printk("%04x %04x %d %d\n", identify[49], 1 << 8, identify[49] & (1 << 8), (identify[49] & (1 << 8)) != 0);
        if ((identify[49] & (1 << 8)) != 0) {
            ide_drives[i].dma = 1;
        }

        u64 max_lba = *(u64 *)(identify + 100);

        if ((identify[83] & (1 << 10)) != 0) {
            ide_drives[i].lba48 = 1;
            ide_drives[i].max_lba = max_lba;
        }

        printk("hard disk %s %s size: %u MB\n", ide_drives[i].dma == 1 ? "DMA" : "",
               ide_drives[i].lba48 == 1 ? "LBA48" : "LBA28", (max_lba * 512) >> 20);
    }
}

void ata_init() {
    // 初始化hard_disk与中断函数之间的信号量

    disk_request_t r;
    r.dev = 0;
    r.buf = (void *)identify;
    r.count = 1;
    r.pos = 0;
    r.command = DISK_REQ_IDENTIFY;

    send_disk_request(&r);

    // 第49个word的第8个bit位表示是否支持DMA
    // 第83个word的第10个bit位表示是否支持LBA48，为1表示支持。
    // 第100~103个word的八个字节表示user的LBA最大值
    printk("%04x %04x %d %d\n", identify[49], 1 << 8, identify[49] & (1 << 8), (identify[49] & (1 << 8)) != 0);
    if ((identify[49] & (1 << 8)) != 0) {
        printk("support DMA\n");
    }

    if ((identify[83] & (1 << 10)) != 0) {
        printk("support LBA48\n");
        u64 lba = *(u64 *)(identify + 100);
        printk("hard disk size: %u MB\n", (lba * 512) >> 20);
    }

    // TODO REMOVE
    mbr_buf = kmalloc(SECT_SIZE, 0);
    r.command = DISK_REQ_READ;
    r.pos = 0;
    r.count = 1;
    r.buf = mbr_buf;
    send_disk_request(&r);
    uint16_t *p = (uint16_t *)mbr_buf;
    for (int i = 0; i < 256; i++) {
        if (i % 12 == 0) {
            printk("\n[%03d] ", i * 2);
        }
        printk("%04x.", p[i]);
    }
}

#if 0
void ata_read_identify_old(int dev) {  // 这里所用的dev是逻辑编号 ATA0、ATA1下的Master、Salve的dev分别为0,1,2,3
    // void send_disk_request();
    // send_disk_request();
    // DECLARE_WAIT_QUEUE_HEAD(wq_head);
    // DECLARE_WAIT_QUEUE(wait, current);
    // add_wait_queue(&wq_head, &wait);
    // ide_pci_controller.task = current;

    outb(0x00, REG_CTL(dev));
    outb(0x00 | ((dev & 0x01) << 4), REG_DEVICE(dev));  // 根据文档P113，这里不用指定bit5, bit7，直接指示DRIVE就行

    unsigned long flags;
    irq_save(flags);

    outb(ATA_CMD_IDENTIFY, REG_CMD(dev));
    wait_on_ide();

    irq_restore(flags);

    insw(REG_DATA(dev), identify, SECT_SIZE / sizeof(u16));

    // 第49个word的第8个bit位表示是否支持DMA
    // 第83个word的第10个bit位表示是否支持LBA48，为1表示支持。
    // 第100~103个word的八个字节表示user的LBA最大值
    printk("%04x %04x %d %d\n", identify[49], 1 << 8, identify[49] & (1 << 8), (identify[49] & (1 << 8)) != 0);
    if ((identify[49] & (1 << 8)) != 0) {
        printk("support DMA\n");
    }

    if ((identify[83] & (1 << 10)) != 0) {
        printk("support LBA48\n");

        u64 lba = *(u64 *)(identify + 100);
        printk("hard disk size: %u MB\n", (lba * 512) >> 20);
    }

    printk("bus iobase %x cmd %x status %x prdt %x \n", ide_pci_controller.bus_iobase, ide_pci_controller.bus_cmd,
           ide_pci_controller.bus_status, ide_pci_controller.bus_prdt);

    // TODO REMOVE
    mbr_buf = kmalloc(SECT_SIZE, 0);
    // ata_test(0);
    sleep_on_ide();
    // ata_pio_read_ext(0, 0, 1, ATA_TIMEOUT, mbr_buf);
    uint16_t *p = (uint16_t *)mbr_buf;
    for (int i = 0; i < 256; i++) {
        if (i % 12 == 0) {
            printk("\n[%03d] ", i);
        }
        printk("%04x ", p[i]);
    }
}
#endif
// ATA_CMD_READ_DMA_EXT
void ata_dma_read_ext(int dev, uint64_t pos, uint16_t count, void *dest) {
    // Intel®
    //  82801CA (ICH3), 82801BA
    // (ICH2), 82801AA (ICH), and 82801AB
    // (ICH0) IDE Controller
    // Programmer’s Reference Manua
    // Page 25. Table 23. BMIC1 and BMIC2

    // • The Bus Master Read/Write Control bit【第3位】 shall set the transfer direction for DMA transfers. This
    // bit must NOT be changed when the bus master function is active. While an Ultra DMA transfer
    // is in progress, this bit will be READ ONLY. The bit will return to read/write once the
    // synchronous DMA transfer has been completed or halted.

    // • The Start/Stop Bus Master bit【第0位】 shall be the control method to start or stop the DMA transfer
    // engine. When this bit is set to 1, bus master operation starts. The controller transfers data
    // between the IDE device and memory only while this bit is set. Master operation can be stopped
    // by writing a 0 to this bit. This results in all state information being lost (i.e., master mode
    // operation cannot be stopped and then resumed).

    // 停止DMA，并设置为读(这里的WRITE是对DMA控制器来说)
    outb(PCI_IDE_CMD_WRITE | PCI_IDE_CMD_STOP, ide_pci_controller.bus_cmd);

    // 配置描述符表
    unsigned long dest_paddr = va2pa(dest);
    ide_pci_controller.prdt[0].phys_addr = dest_paddr;
    ide_pci_controller.prdt[0].byte_count = SECT_SIZE;
    ide_pci_controller.prdt[0].reserved = 0;
    ide_pci_controller.prdt[0].eot = 1;
    outl(va2pa(ide_pci_controller.prdt), ide_pci_controller.bus_prdt);

    printk("paddr: %x prdt: %x %x prdte %x %x\n", dest_paddr, ide_pci_controller.prdt, va2pa(ide_pci_controller.prdt),
           ide_pci_controller.prdt[0].phys_addr, *(((unsigned int *)ide_pci_controller.prdt) + 1));

    // 清除中断位和错误位
    // 这里清除的方式是是设置1后清除
    outb(PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, ide_pci_controller.bus_status);

    // 不再设置nIEN，DMA需要中断
    outb(0x00, REG_CTL(dev));

    // 等待硬盘不BUSY
    while (inb(REG_STATUS(dev)) & ATA_STATUS_BSY) {
        nop();
    }

    // 选择DRIVE
    outb(ATA_LBA48_DEVSEL(dev), REG_DEVICE(dev));

    // 先写扇区数的高字节
    outb((count >> 8) & 0xFF, REG_NSECTOR(dev));

    // 接着写LBA48，高三个字节
    outb((pos >> 24) & 0xFF, REG_LBAL(dev));
    outb((pos >> 32) & 0xFF, REG_LBAM(dev));
    outb((pos >> 40) & 0xFF, REG_LBAH(dev));

    // 再写扇区数的低字节
    outb((count >> 0) & 0xFF, REG_NSECTOR(dev));

    // 接着写LBA48，低三个字节
    outb((pos >> 0) & 0xFF, REG_LBAL(dev));
    outb((pos >> 8) & 0xFF, REG_LBAM(dev));
    outb((pos >> 16) & 0xFF, REG_LBAH(dev));

    // 等待硬盘READY
    while (inb(REG_STATUS(dev)) & ATA_STATUS_RDY == 0) {
        nop();
    }

    outb(ATA_CMD_READ_DMA_EXT, REG_CMD(dev));

    // 这一句非常重要，如果不加这一句
    // 在qemu中用DMA的方式读数据就会读不到数据，而只触是发中断，然后寄存器（Bus Master IDE Status
    // Register）的值会一直是5 也就是INTERRUPT和和ACTIVE位是1，正常应该是4，也就是只有INTERRUPT位为1
    // 在bochs中则加不加这一句不会有影响，都能正常读到数据
    unsigned int v = pci_read_config_word(pci_cmd(ide_pci_controller.pci, PCI_COMMAND));
    printk(" ide pci command %04x\n", v);
    pci_write_config_word(v | PCI_COMMAND_MASTER, pci_cmd(ide_pci_controller.pci, PCI_COMMAND));

    // 指定DMA操作为读取硬盘操作，内核用DMA读取，对硬盘而言是写出
    // 并设置DMA的开始位，开始DMA
    outb(PCI_IDE_CMD_START, ide_pci_controller.bus_cmd);
}

// TODO
int ata_dma_stop() {
    uint8_t x = inb(ide_pci_controller.bus_cmd);
    x &= ~PCI_IDE_CMD_START;
    outb(x, ide_pci_controller.bus_cmd);

    uint8_t status = inb(ide_pci_controller.bus_status);
    outb(status | PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, ide_pci_controller.bus_status);

    // TODO
    if (status & PCI_IDE_STATUS_ERR) {
        return -1;
    }

    return 0;
}

// ATA_CMD_READ_PIO_EXT
int ata_pio_read_ext(int dev, uint64_t pos, uint16_t count, int timeout, void *dest) {
    // PIO读，禁用中断
    outb(ATA_CTL_NIEN, REG_CTL(dev));

    // 等待硬盘不BUSY
    while (inb(REG_STATUS(dev)) & ATA_STATUS_BSY) {
        nop();
    }

    // 选择DRIVE
    outb(ATA_LBA48_DEVSEL(dev), REG_DEVICE(dev));

    // 先写扇区数的高字节
    outb((count >> 8) & 0xFF, REG_NSECTOR(dev));

    // 接着写LBA48，高三个字节q
    outb((pos >> 24) & 0xFF, REG_LBAL(dev));
    outb((pos >> 32) & 0xFF, REG_LBAM(dev));
    outb((pos >> 40) & 0xFF, REG_LBAH(dev));

    // 再写扇区数的低字节
    outb((count >> 0) & 0xFF, REG_NSECTOR(dev));

    // 接着写LBA48，低三个字节
    outb((pos >> 0) & 0xFF, REG_LBAL(dev));
    outb((pos >> 8) & 0xFF, REG_LBAM(dev));
    outb((pos >> 16) & 0xFF, REG_LBAH(dev));

    while (inb(REG_STATUS(dev)) & ATA_STATUS_RDY == 0) {
        nop();
    }

    outb(ATA_CMD_READ_PIO_EXT, REG_CMD(dev));

    while (timeout > 0) {
        timeout--;

        u8 status = inb(REG_STATUS(dev));
        if ((status & ATA_STATUS_BSY) == 0 && (status & ATA_STATUS_DRQ) != 0) {
            break;
        }

        asm("sti;hlt;");
    }
    asm("cli");

    if (timeout == 0) {
        return -1;
    }

    insl(REG_DATA(dev), dest, (SECT_SIZE * count) / sizeof(uint32_t));

    return 0;
}

uint8_t ata_pci_bus_status() {
    uint8_t st = 0;
    st = inb(ide_pci_controller.bus_status);

    outb(PCI_IDE_STATUS_INTR, ide_pci_controller.bus_status);

    return st;
}

unsigned int ATA_CHL0_CMD_BASE = 0x1F0;
unsigned int ATA_CHL1_CMD_BASE = 0x170;

unsigned int ATA_CHL0_CTL_BASE = 0x3F6;
unsigned int ATA_CHL1_CTL_BASE = 0x376;