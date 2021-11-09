/*
 * ------------------------------------------------------------------------
 *   File Name: ata.c
 *      Author: Zhao Yanbai
 *              2021-11-10 13:55:27 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <ata.h>
#include <io.h>
#include <system.h>
#include <types.h>

// 本程序参考文档《AT Attachment with Packet Interface - 6》

// https://wiki.osdev.org/ATA_PIO_Mode
// To use the IDENTIFY command, select a target drive by sending 0xA0 for the master drive, or 0xB0 for the slave,
// to the "drive select" IO port. On the Primary bus, this would be port 0x1F6. Then set the Sectorcount, LBAlo,
// LBAmid, and LBAhi IO ports to 0 (port 0x1F2 to 0x1F5). Then send the IDENTIFY command (0xEC) to the Command IO
// port (0x1F7).
// Then read the Status port (0x1F7) again. If the value read is 0, the drive does not exist. For any
// other value: poll the Status port (0x1F7) until bit 7 (BSY, value = 0x80) clears. Because of some ATAPI drives
// that do not follow spec, at this point you need to check the LBAmid and LBAhi ports (0x1F4 and 0x1F5) to see if
// they are non-zero. If so, the drive is not ATA, and you should stop polling. Otherwise, continue polling one of
// the Status ports until bit 3 (DRQ, value = 8) sets, or until bit 0 (ERR, value = 1) sets. At that point, if ERR
// is clear, the data is ready to read from the Data port (0x1F0). Read 256 16-bit values, and store them.
//
// ATAPI的情况暂时不用考虑，因为不是硬盘相关的
// https://wiki.osdev.org/ATAPI
// ATAPI refers to devices that use the Packet Interface of the ATA6 (or higher) standard command set. It is
// basically a way to issue SCSI commands to a CD-ROM, CD-RW, DVD, or tape drive, attached to the ATA bus.
//
// 总结来说，仅考虑ATA硬盘的情况
// 一个IDE接口能接Master、Slave两个DRIVE。
// 一个PC机上通常有两个IDE接口(IDE0, IDE1或ATA0, ATA1)，通常称通道0、1
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
u16 identify[256];
void ata_read_identify(int dev) {  // 这里所用的dev是逻辑编号 ATA0、ATA1下的Master、Salve的dev分别为0,1,2,3
    outb(0x00 | ((dev & 0x01) << 4), REG_DEVICE(dev));  // 根据文档P113，这里不用指定bit5, bit7，直接指示DRIVE就行
    outb(ATA_CMD_IDENTIFY, REG_CMD(dev));
    while (1) {
        u8 status = inb(REG_STATUS(dev));
        printk("hard disk status: %x %x\n", status, REG_STATUS(dev));
        if ((status & ATA_STATUS_BSY) == 0 && (status & ATA_STATUS_DRQ) != 0) {
            break;
        }
    }

    // u16 *identify = (u16 *)kmalloc(ATA_NSECTOR, 0);
    insw(REG_DATA(dev), identify, 512 / sizeof(u16));

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
}

unsigned int ATA_CHL0_CMD_BASE = 0x1F0;
unsigned int ATA_CHL1_CMD_BASE = 0x170;

unsigned int ATA_CHL0_CTL_BASE = 0x3F6;
unsigned int ATA_CHL1_CTL_BASE = 0x376;