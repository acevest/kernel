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

ide_drive_t ide_drives[MAX_IDE_DRIVE_CNT];

#define ATA_TIMEOUT 10  // 10次时钟中断

void ata_dma_read_ext(int drv, uint64_t pos, uint16_t count, void *dest);
int ata_pio_read_ext(int drv, uint64_t pos, uint16_t count, int timeout, void *dest);

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
void ata_send_read_identify_cmd(int drv) {}

void ata_read_data(int drv, int sect_cnt, void *dst) { insl(REG_DATA(drv), dst, (512 * sect_cnt) / sizeof(uint32_t)); }

// 这里所用的drv是逻辑编号 ATA0、ATA1下的Master、Salve的drv分别为0,1,2,3
void ata_read_identify(int drv, int disable_intr) {
    uint8_t ctlv = 0x00;
    if (disable_intr != 0) {
        ctlv |= ATA_CTL_NIEN;
    }
    outb(ctlv, REG_CTL(drv));
    outb(0x00 | ((drv & 0x01) << 4), REG_DEVICE(drv));  // 根据文档P113，这里不用指定bit5, bit7，直接指示DRIVE就行
    outb(ATA_CMD_IDENTIFY, REG_CMD(drv));
}

void ata_read_identity_string(const uint16_t *identify, int bgn, int end, char *buf) {
    const char *p = (const char *)(identify + bgn);
    int i = 0;
    for (; i <= (end - bgn); i++) {
        buf[2 * i + 1] = p[0];
        buf[2 * i + 0] = p[1];
        p += 2;
    }
    buf[i] = 0;
}

void ata_read_partions(ide_part_t *part, const char *buf) {
    int offset = PARTITION_TABLE_OFFSET;
    const char *p = buf + offset;

    for (int i = 0; i < 4; i++) {
        part->flags = (uint8_t)p[0];
        part->type = (uint8_t)p[4];
        part->lba_start = *((uint32_t *)(p + 8));
        uint32_t size = *((uint32_t *)(p + 12));
        part->lba_end = part->lba_start + size;

        printk("part[%d] %02X %02X %u %u\n", i, part->flags, part->type, part->lba_start,
               part->lba_end == 0 ? 0 : part->lba_end - 1);

        // 这里应该再判断一下part->flags，如果是扩展分区还需要再读取
        // 先这样实现

        p += 16;  // 每个分区16个字节
        part++;
    }
}

// 《AT Attachment 8 - ATA/ATAPI Command Set》
void ide_ata_init() {
    for (int i = 0; i < MAX_IDE_DRIVE_CNT; i++) {
        int drv_no = i;
        int channel = drv_no >> 1;
        memset(ide_drives + i, 0, sizeof(ide_drive_t));

        ide_drive_t *drv = ide_drives + drv_no;
        drv->drv_no = drv_no;
        drv->ide_pci_controller = ide_pci_controller + channel;

        // https://wiki.osdev.org/ATA_PIO_Mode
        // To use the IDENTIFY command, select a target drive by sending 0xA0 for the master drive, or 0xB0 for the
        // slave, to the "drive select" IO port. On the Primary bus, this would be port 0x1F6. Then set the Sectorcount,
        // LBAlo, LBAmid, and LBAhi IO ports to 0 (port 0x1F2 to 0x1F5). Then send the IDENTIFY command (0xEC) to the
        // Command IO port (0x1F7). Then read the Status port (0x1F7) again. If the value read is 0, the drive does not
        // exist. For any other value: poll the Status port (0x1F7) until bit 7 (BSY, value = 0x80) clears. Because of
        // some ATAPI drives that do not follow spec, at this point you need to check the LBAmid and LBAhi ports (0x1F4
        // and 0x1F5) to see if they are non-zero. If so, the drive is not ATA, and you should stop polling. Otherwise,
        // continue polling one of the Status ports until bit 3 (DRQ, value = 8) sets, or until bit 0 (ERR, value = 1)
        // sets. At that point, if ERR is clear, the data is ready to read from the Data port (0x1F0). Read 256 16-bit
        // values, and store them.
        //
        // ATAPI的情况暂时不用考虑，因为不是硬盘相关的
        // https://wiki.osdev.org/ATAPI
        // ATAPI refers to devices that use the Packet Interface of the ATA6 (or higher) standard command set. It is
        // basically a way to issue SCSI commands to a CD-ROM, CD-RW, DVD, or tape drive, attached to the ATA bus.
        //
        // 总结来说，仅考虑ATA硬盘的情况
        // 一个IDE接口能接Master、Slave两个DRIVE。
        // 一个PC机上通常有两个IDE接口(IDE0, IDE1或ATA0, ATA1)，通常称通道0、1
        //
        // 对于同一个IDE通道的两个DRIVE，共享同一组寄存器，它们之间的区分是通过Device寄存器的第4个bit位来实现的。0为Master，1为Slave
        //
        // 使用IDENTIFY命令步骤:
        //  1. 选择DRIVE，发送0xA0选择master，发送0xB0选择slave。(发送 0xE0 | (drive << 4)到Device寄存器)
        //  2. 发送0到该DRIVE所在通道的寄存器NSECTOR, LBAL, LBAM, LBAH
        //  3. 发送IDENTIFY(0xEC)命令到该通道的命令寄存器
        // 检查status寄存器：
        //  1. 若为0，就认为没有IDE
        //  2. 等到status的BSY位清除
        //  3. 等到status的DRQ位或ERR位设置

        ata_read_identify(drv_no, 1);

        uint8_t status = inb(REG_STATUS(drv_no));
        if (status == 0 || (status & ATA_STATUS_ERR) || (status & ATA_STATUS_RDY == 0)) {
            drv->present = 0;
            continue;
        } else {
            drv->present = 1;
        }

        printk("ata[%d] status %x %s exists\n", i, status, drv->present == 1 ? "" : "not");
        insl(REG_DATA(drv_no), identify, SECT_SIZE / sizeof(uint32_t));

        // 第49个word的第8个bit位表示是否支持DMA
        // 第83个word的第10个bit位表示是否支持LBA48，为1表示支持。
        // 第100~103个word的八个字节表示user的LBA最大值
        // printk("%04x %04x %d %d\n", identify[49], 1 << 8, identify[49] & (1 << 8), (identify[49] & (1 << 8)) != 0);
        if ((identify[49] & (1 << 8)) != 0) {
            drv->dma = 1;
        }

        u64 max_lba = *(u64 *)(identify + 100);

        if ((identify[83] & (1 << 10)) != 0) {
            drv->lba48 = 1;
            drv->max_lba = max_lba;
        }
#if 0
        uint16_t i80 = identify[80];
        if (i80 & (1 << 8)) {
            printk("ATA8-ACS ");
        }
        if (i80 & (1 << 7)) {
            printk("ATA/ATAPI-7 ");
        }
        if (i80 & (1 << 6)) {
            printk("ATA/ATAPI-6 ");
        }
        if (i80 & (1 << 5)) {
            printk("ATA/ATAPI-5 ");
        }
        if (i80 & (1 << 4)) {
            printk("ATA/ATAPI-4 ");
        }

        printk(" %02x\n", identify[81]);
#endif
        printk("Ultra DMA modes: %04x\n", identify[88]);

#if 0
        uint16_t x = identify[222];
        uint16_t tt = x >> 12;
        switch (tt) {
        case 0:
            printk("parallel");
            break;
        case 1:
            printk("serial");
            break;
        default:
            printk("reserved");
            break;
        }
#endif
        printk("hard disk %s %s size: %u MB\n", drv->dma == 1 ? "DMA" : "", drv->lba48 == 1 ? "LBA48" : "LBA28",
               (max_lba * 512) >> 20);

        char s[64];
        ata_read_identity_string(identify, 10, 19, s);
        printk("SN: %s\n", s);

        ata_read_identity_string(identify, 23, 26, s);
        printk("Firmware Revision: %s\n", s);

        ata_read_identity_string(identify, 27, 46, s);
        printk("HD Model: %s\n", s);

        // 0 代表整个硬盘
        // 1~4代表各个主分区
        // 5~15 代表各个逻辑分区
        drv->partions[0].flags = 0x00;
        drv->partions[0].lba_start = 0;
        drv->partions[0].lba_end = drv->max_lba;
    }
}

// ext_lba: 在MBR中的扩展分区记录里的偏移地址
// offset_lba: 在扩展分区记录里的扩展分区的偏移地址
void read_partition_table(ide_drive_t *drv, uint32_t ext_lba, uint32_t offset_lba, int depth) {
    uint32_t base = depth <= 1 ? 0 : ext_lba;

    disk_request_t r;
    char *sect = kmalloc(SECT_SIZE, 0);
    r.dev = MAKE_DEV(DEV_MAJOR_IDE0 + (drv->drv_no >> 1), drv->drv_no % 2);
    r.command = DISK_REQ_READ;
    r.pos = base + offset_lba;
    r.count = 1;
    r.buf = sect;
    send_disk_request(&r);

    ide_part_t *part = 0;
    uint32_t part_id = 0;
    if (depth == 0) {
        // MBR里的分区占据 [1,4]
        part_id = 1;
    } else {
        // 扩展分区里的逻辑分区占据 [5,MAX_IDE_PARTIONS)
        part_id = 5 + depth - 1;
    }

    const char *p = sect + PARTITION_TABLE_OFFSET;
    for (int i = 0; i < 4; i++) {
        if (part_id >= MAX_IDE_PARTIONS) {
            break;
        }
        part = drv->partions + part_id;
        part->flags = (uint8_t)p[0];
        part->type = (uint8_t)p[4];
        part->lba_start = *((uint32_t *)(p + 8));
        uint32_t size = *((uint32_t *)(p + 12));
        part->lba_end = part->lba_start + size;
        part->lba_end = size;

        if (part->type == 0x00) {
            continue;
        }

        if (part->type == 0x05) {
            ext_lba = ext_lba != 0 ? ext_lba : part->lba_start;
            read_partition_table(drv, ext_lba, part->lba_start, depth + 1);
        } else {
            printk("part[%d] %02X %u %u\n", part_id + drv->drv_no * MAX_IDE_PARTIONS, part->type, part->lba_start,
                   size);
        }

        part_id++;

        // 每个分区16个字节
        p += 16;
    }

    kfree(sect);
}

void ide_read_partions() {
    for (int i = 0; i < MAX_IDE_DRIVE_CNT; i++) {
        ide_drive_t *drv = ide_drives + i;
        int channel = i >> 1;

        if (0 == drv->present) {
            continue;
        }

        printk("read ide drive %u\n", drv->drv_no);
        read_partition_table(drv, 0, 0, 0);
        printk("--------------\n");
    }
}

void ide_disk_read(dev_t dev, uint32_t sect_nr, uint32_t count, char *buf) {
    ide_drive_t *drv = ide_get_drive(dev);
    uint64_t lba_offset = drv->partions[DEV_MINOR((dev))].lba_start;

    disk_request_t r;
    r.dev = dev;
    r.command = DISK_REQ_READ;
    r.pos = lba_offset + sect_nr;
    r.count = count;
    r.buf = buf;
    send_disk_request(&r);
}

// ATA_CMD_READ_DMA_EXT
void ata_dma_read_ext(int drv, uint64_t pos, uint16_t count, void *dest) {
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

    int channel = (drv >> 1) & 0x01;
    assert(channel == 0 || channel == 1);
    ide_pci_controller_t *ide_ctrl = ide_pci_controller + channel;

    // 停止DMA
    outb(PCI_IDE_CMD_STOP, ide_ctrl->bus_cmd);

    // 配置描述符表
    unsigned long dest_paddr = va2pa(dest);

    // 不能跨64K边界
    const uint32_t size = count * SECT_SIZE;
    const uint32_t _64K = 1 << 16;
    assert(((dest_paddr + size) & _64K) == (dest_paddr & _64K));

    ide_ctrl->prdt[0].phys_addr = dest_paddr;
    ide_ctrl->prdt[0].byte_count = size;
    ide_ctrl->prdt[0].reserved = 0;
    ide_ctrl->prdt[0].eot = 1;
    outl(va2pa(ide_ctrl->prdt), ide_ctrl->bus_prdt);

    // printk("paddr: %x prdt: %x %x prdte %x %x\n", dest_paddr, ide_ctrl->prdt,
    // va2pa(ide_ctrl->prdt),
    //        ide_ctrl->prdt[0].phys_addr, *(((unsigned int *)ide_ctrl->prdt) + 1));

    // 清除中断位和错误位
    // 这里清除的方式是是设置1后清除
    outb(PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, ide_ctrl->bus_status);

    // 不再设置nIEN，DMA需要中断
    outb(0x00, REG_CTL(drv));

    // 等待硬盘不BUSY
    while (inb(REG_STATUS(drv)) & ATA_STATUS_BSY) {
        nop();
    }

    // 选择DRIVE
    outb(ATA_LBA48_DEVSEL(drv), REG_DEVICE(drv));

    // 先写扇区数的高字节
    outb((count >> 8) & 0xFF, REG_NSECTOR(drv));

    // 接着写LBA48，高三个字节
    outb((pos >> 24) & 0xFF, REG_LBAL(drv));
    outb((pos >> 32) & 0xFF, REG_LBAM(drv));
    outb((pos >> 40) & 0xFF, REG_LBAH(drv));

    // 再写扇区数的低字节
    outb((count >> 0) & 0xFF, REG_NSECTOR(drv));

    // 接着写LBA48，低三个字节
    outb((pos >> 0) & 0xFF, REG_LBAL(drv));
    outb((pos >> 8) & 0xFF, REG_LBAM(drv));
    outb((pos >> 16) & 0xFF, REG_LBAH(drv));

    // 等待硬盘READY
    while (inb(REG_STATUS(drv)) & ATA_STATUS_RDY == 0) {
        nop();
    }

    outb(ATA_CMD_READ_DMA_EXT, REG_CMD(drv));

    // 这一句非常重要，如果不加这一句
    // 在qemu中用DMA的方式读数据就会读不到数据，而只触是发中断，然后寄存器（Bus Master IDE Status
    // Register）的值会一直是5 也就是INTERRUPT和和ACTIVE位是1，正常应该是4，也就是只有INTERRUPT位为1
    // 在bochs中则加不加这一句不会有影响，都能正常读到数据
    unsigned int v = pci_read_config_word(pci_cmd(ide_ctrl->pci, PCI_COMMAND));
    // printk(" ide pci command %04x\n", v);
    pci_write_config_word(v | PCI_COMMAND_MASTER, pci_cmd(ide_ctrl->pci, PCI_COMMAND));
    // pci_write_config_word(v, pci_cmd(ide_ctrl->pci, PCI_COMMAND));

    // 指定DMA操作为读取硬盘操作，内核用DMA读取，对硬盘而言是写出
    // 并设置DMA的开始位，开始DMA
    outb(PCI_IDE_CMD_WRITE | PCI_IDE_CMD_START, ide_ctrl->bus_cmd);
}

// TODO
int ata_dma_stop(int channel) {
    ide_pci_controller_t *ide_ctrl = ide_pci_controller + channel;

    uint8_t x = inb(ide_ctrl->bus_cmd);
    x &= ~PCI_IDE_CMD_START;
    outb(x, ide_ctrl->bus_cmd);

    uint8_t status = inb(ide_ctrl->bus_status);
    outb(status | PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, ide_ctrl->bus_status);

    // TODO
    if (status & PCI_IDE_STATUS_ERR) {
        return -1;
    }

    return 0;
}

// ATA_CMD_READ_PIO_EXT
int ata_pio_read_ext(int drv, uint64_t pos, uint16_t count, int timeout, void *dest) {
    // PIO读，禁用中断
    outb(ATA_CTL_NIEN, REG_CTL(drv));

    // 等待硬盘不BUSY
    while (inb(REG_STATUS(drv)) & ATA_STATUS_BSY) {
        nop();
    }

    // 选择DRIVE
    outb(ATA_LBA48_DEVSEL(drv), REG_DEVICE(drv));

    // 先写扇区数的高字节
    outb((count >> 8) & 0xFF, REG_NSECTOR(drv));

    // 接着写LBA48，高三个字节q
    outb((pos >> 24) & 0xFF, REG_LBAL(drv));
    outb((pos >> 32) & 0xFF, REG_LBAM(drv));
    outb((pos >> 40) & 0xFF, REG_LBAH(drv));

    // 再写扇区数的低字节
    outb((count >> 0) & 0xFF, REG_NSECTOR(drv));

    // 接着写LBA48，低三个字节
    outb((pos >> 0) & 0xFF, REG_LBAL(drv));
    outb((pos >> 8) & 0xFF, REG_LBAM(drv));
    outb((pos >> 16) & 0xFF, REG_LBAH(drv));

    while (inb(REG_STATUS(drv)) & ATA_STATUS_RDY == 0) {
        nop();
    }

    outb(ATA_CMD_READ_PIO_EXT, REG_CMD(drv));

    while (timeout > 0) {
        timeout--;

        u8 status = inb(REG_STATUS(drv));
        if ((status & ATA_STATUS_BSY) == 0 && (status & ATA_STATUS_DRQ) != 0) {
            break;
        }

        asm("sti;hlt;");
    }
    asm("cli");

    if (timeout == 0) {
        return -1;
    }

    insl(REG_DATA(drv), dest, (SECT_SIZE * count) / sizeof(uint32_t));

    return 0;
}

// uint8_t ata_pci_bus_status() {
//     uint8_t st = 0;
//     st = inb(ide_pci_controller.bus_status);

//     outb(PCI_IDE_STATUS_INTR, ide_pci_controller.bus_status);

//     return st;
// }
