/*
 * ------------------------------------------------------------------------
 *   File Name: ide.h
 *      Author: Zhao Yanbai
 *              Tue May 27 22:51:50 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <pci.h>
#include <system.h>
#include <task.h>

// extern unsigned int HD_CHL0_CMD_BASE;
// extern unsigned int HD_CHL1_CMD_BASE;

// // CTL其实不用BASE，只有一个寄存器
// // 这么写纯粹是为了代码好看
// // DEVICE CONTROL REGISTER
// // bit7: HOB    High Order Byte (defined by 48-bit Address feature set).
// // bit[3-6]: -
// // bit2: SRST   Software Reset
// // bit1: IEN    Interrupt Enable
// // bit0: 0
// extern unsigned int HD_CHL0_CTL_BASE;
// extern unsigned int HD_CHL1_CTL_BASE;

// #define HD_DATA 0
// #define HD_FEATURES 1
// #define HD_ERR 1
// #define HD_ERR_BB 0x80
// #define HD_ERR_ECC 0x40
// #define HD_ERR_ID 0x10
// #define HD_ERR_AC 0x04
// #define HD_ERR_TK 0x02
// #define HD_ERR_DM 0x01
// #define HD_NSECTOR 2
// #define HD_LBAL 3
// #define HD_LBAM 4
// #define HD_LBAH 5

// // DEVICE寄存器
// // bit7: 1
// // bit6: L 如果为1，LBA Mode
// // bit5: 1
// // bit4: DRIVE
// // bit[0, 3] HS 如果L为0就是磁头号Head Number，如果L为1，则为LBA的24-27位
// #define HD_DEVICE 6

// #define HD_CMD 7

// // bit7: BSY Busy. If BSY==1, no other bits in the register are valid
// // bit6: DRDY Drive Ready.
// // bit5: DF/SE Device Fault / Stream Error
// // bit4: # Command dependent. (formerly DSC bit)
// // bit3: DRQ Data Request. (ready to transfer data)
// // bit2: - Obsolete
// // bit1: - Obsolete
// // bit0: ERR
// #define HD_STATUS 7              /* controller status */
// #define HD_STATUS_BSY 0x80       /* controller busy */
// #define HD_STATUS_RDY 0x40       /* drive ready */
// #define HD_STATUS_WF 0x20        /* write fault */
// #define HD_STATUS_SEEK_CMPT 0x10 /* seek complete */
// #define HD_STATUS_DRQ 0x08       /* data transfer request */
// #define HD_STATUS_CRD 0x04       /* correct data */
// #define HD_STATUS_IDX 0x02       /* index pulse */
// #define HD_STATUS_ERR 0x01       /* error */

// #define HD_CMD_IDLE 0x00
// #define HD_CMD_RECALIBRATE 0x10
// #define HD_CMD_READ_PIO 0x20     /* read data */
// #define HD_CMD_READ_PIO_EXT 0x24 /* read data (LBA-48 bit)*/
// #define HD_CMD_READ_DMA 0xC8
// #define HD_CMD_READ_DMA_EXT 0x25 /* read data DMA LBA48 */
// #define HD_CMD_WRITE_PIO 0x30
// #define HD_CMD_WRITE_PIO_EXT 0x34
// #define HD_CMD_WRITE_DMA 0xCA
// #define HD_CMD_WRITE_DMA_EXT 0X35
// #define HD_CMD_READ_VERIFY 0x40
// #define HD_CMD_FORMAT 0x50
// #define HD_CMD_SEEK 0x70
// #define HD_CMD_DIAG 0x90
// #define HD_CMD_SPECIFY 0x91
// #define HD_CMD_IDENTIFY_PACKET 0xA1
// #define HD_CMD_IDENTIFY 0xEC

// #define HD_CTL 0
// #define HD_CTL_HOB 0x80 /* high order byte (LBA-48bit) */
// //#define     HD_CTL_NOECC        0x40  /* disable ecc retry */
// //#define     HD_CTL_EIGHTHEADS   0x08  /* more than 8 heads */
// #define HD_CTL_SRST 0x04 /* soft reset controller */
// #define HD_CTL_NIEN 0x02 /* disable interrupts */

// #define HD_GET_CHL(dev) (0) /* only support channel 0 */
// #define HD_GET_DEV(dev) (0) /* only support one hard disk */

// #define REG_CMD_BASE(dev, offset) (HD_GET_CHL(dev) ? (HD_CHL1_CMD_BASE + offset) : (HD_CHL0_CMD_BASE + offset))
// #define REG_CTL_BASE(dev, offset) (HD_GET_CHL(dev) ? (HD_CHL1_CTL_BASE + offset) : (HD_CHL0_CTL_BASE + offset))

// #define REG_DATA(dev) REG_CMD_BASE(dev, HD_DATA)
// #define REG_ERR(dev) REG_CMD_BASE(dev, HD_ERR)
// #define REG_NSECTOR(dev) REG_CMD_BASE(dev, HD_NSECTOR)
// #define REG_LBAL(dev) REG_CMD_BASE(dev, HD_LBAL)
// #define REG_LBAM(dev) REG_CMD_BASE(dev, HD_LBAM)
// #define REG_LBAH(dev) REG_CMD_BASE(dev, HD_LBAH)
// #define REG_DEVICE(dev) REG_CMD_BASE(dev, HD_DEVICE)
// #define REG_STATUS(dev) REG_CMD_BASE(dev, HD_STATUS)
// #define REG_FEATURES(dev) REG_CMD_BASE(dev, HD_FEATURES)

// #define REG_CMD(dev) REG_CMD_BASE(dev, HD_CMD)
// #define REG_CTL(dev) REG_CTL_BASE(dev, HD_CTL)

// #define hd_rd_data(dev, buf, count) hd_rd_port(REG_DATA(dev), buf, count)

// #define hd_bsy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_BSY))
// #define hd_rdy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_RDY))
// #define hd_drq(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_DRQ))
// #define hd_err(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_ERR))

// #define ATA_IDENT_DEVTYPE 0
// #define ATA_IDENT_CYLINDERS 2
// #define ATA_IDENT_HEADS 6
// #define ATA_IDENT_SECTORS 12
// #define ATA_IDENT_SERIAL 20
// #define ATA_IDENT_MODEL 54
// #define ATA_IDENT_CAPABILITIES 98
// #define ATA_IDENT_FIELDVALID 106
// #define ATA_IDENT_MAX_LBA 120
// #define ATA_IDENT_COMMANDSETS 164
// #define ATA_IDENT_MAX_LBA_EXT 200

#define PCI_IDE_CMD 0
#define PCI_IDE_CMD_STOP 0x00
#define PCI_IDE_CMD_READ 0x00
#define PCI_IDE_CMD_START 0x01
#define PCI_IDE_CMD_WRITE 0x08
#define PCI_IDE_STATUS 2
#define PCI_IDE_STATUS_ACT 0x01
#define PCI_IDE_STATUS_ERR 0x02
#define PCI_IDE_STATUS_INTR 0x04
#define PCI_IDE_STATUS_DRV0 0x20
#define PCI_IDE_STATUS_DRV1 0x40
#define PCI_IDE_STATUS_SIMPLEX 0x80
#define PCI_IDE_PRDT 4

// #define PARTITION_CNT 4
// #define PARTITION_TABLE_OFFSET 0x1BE
// #define MAX_SUPPORT_PARTITION_CNT 16

// typedef struct {
//     u64_t lba_start;
//     u64_t lba_end;
// } part_t;

// void ide_do_read(u64_t lba, u32_t scnt, char *buf);
// part_t *ide_get_part(dev_t dev);

// Physical Region Descriptor
typedef struct prdte {
    uint32_t phys_addr;
    uint32_t byte_count : 16;
    uint32_t reserved : 15;
    uint32_t eot : 1;
} prdte_t;
typedef struct _ide_pci_controller {
    pci_device_t *pci;

    unsigned int bus_iobase;
    unsigned int bus_cmd;
    unsigned int bus_status;
    unsigned int bus_prdt;

    prdte_t *prdt;

    // 这里应该改成一个请求链表
    // 先简单实现
    task_union *task;
} ide_pci_controller_t;

void sleep_on_ide();