/*
 * ------------------------------------------------------------------------
 *   File Name: ata.h
 *      Author: Zhao Yanbai
 *              2021-11-10 13:55:29 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

extern unsigned int ATA_CHL0_CMD_BASE;
extern unsigned int ATA_CHL1_CMD_BASE;

// CTL其实不用BASE，只有一个寄存器
// 这么写纯粹是为了代码好看
// DEVICE CONTROL REGISTER
// bit7: HOB    High Order Byte (defined by 48-bit Address feature set).
// bit[3-6]: -
// bit2: SRST   Software Reset
// bit1: IEN    Interrupt Enable
// bit0: 0
extern unsigned int ATA_CHL0_CTL_BASE;
extern unsigned int ATA_CHL1_CTL_BASE;

#define ATA_DATA 0

#define ATA_FEATURES 1

#define ATA_ERR 1
#define ATA_ERR_BB 0x80
#define ATA_ERR_ECC 0x40
#define ATA_ERR_ID 0x10
#define ATA_ERR_AC 0x04
#define ATA_ERR_TK 0x02
#define ATA_ERR_DM 0x01

#define ATA_NSECTOR 2

#define ATA_LBAL 3
#define ATA_LBAM 4
#define ATA_LBAH 5

// DEVICE寄存器
// bit7: Obsolete
// bit6: L 如果为1，LBA Mode
// bit5: Obsolete
// bit4: DRIVE
// bit[0, 3] HS 如果L为0就是磁头号Head Number，如果L为1，则为LBA的24-27位
#define ATA_LBA48_DEVSEL(dev) (0x40 | ((dev & 0x01) << 4))
#define ATA_DEVICE 6

#define ATA_CMD 7

// bit7: BSY Busy. If BSY==1, no other bits in the register are valid
// bit6: DRDY Drive Ready.
// bit5: DF/SE Device Fault / Stream Error
// bit4: # Command dependent. (formerly DSC bit)
// bit3: DRQ Data Request. (ready to transfer data)
// bit2: - Obsolete
// bit1: - Obsolete
// bit0: ERR
#define ATA_STATUS 7              /* controller status */
#define ATA_STATUS_BSY 0x80       /* controller busy */
#define ATA_STATUS_RDY 0x40       /* drive ready */
#define ATA_STATUS_WF 0x20        /* write fault */
#define ATA_STATUS_SEEK_CMPT 0x10 /* seek complete */
#define ATA_STATUS_DRQ 0x08       /* data transfer request */
#define ATA_STATUS_CRD 0x04       /* correct data */
#define ATA_STATUS_IDX 0x02       /* index pulse */
#define ATA_STATUS_ERR 0x01       /* error */

#define ATA_CMD_IDLE 0x00
#define ATA_CMD_RECALIBRATE 0x10
#define ATA_CMD_READ_PIO 0x20     /* read data */
#define ATA_CMD_READ_PIO_EXT 0x24 /* read data (LBA-48 bit)*/
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EXT 0x25 /* read data DMA LBA48 */
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EXT 0X35
#define ATA_CMD_READ_VERIFY 0x40
#define ATA_CMD_FORMAT 0x50
#define ATA_CMD_SEEK 0x70
#define ATA_CMD_DIAG 0x90
#define ATA_CMD_SPECIFY 0x91
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_CTL 0
#define ATA_CTL_HOB 0x80        /* high order byte (LBA-48bit) */
#define ATA_CTL_NOECC 0x40      /* disable ecc retry */
#define ATA_CTL_EIGHTHEADS 0x08 /* more than 8 heads */
#define ATA_CTL_SRST 0x04       /* soft reset controller */
#define ATA_CTL_NIEN 0x02       /* disable interrupts */

#define ATA_GET_CHL(dev) (0) /* only support channel 0 */
#define ATA_GET_DEV(dev) (0) /* only support one hard disk */

#define REG_CMD_BASE(dev, offset) (ATA_GET_CHL(dev) ? (ATA_CHL1_CMD_BASE + offset) : (ATA_CHL0_CMD_BASE + offset))
#define REG_CTL_BASE(dev, offset) (ATA_GET_CHL(dev) ? (ATA_CHL1_CTL_BASE + offset) : (ATA_CHL0_CTL_BASE + offset))

#define REG_DATA(dev) REG_CMD_BASE(dev, ATA_DATA)
#define REG_ERR(dev) REG_CMD_BASE(dev, ATA_ERR)
#define REG_NSECTOR(dev) REG_CMD_BASE(dev, ATA_NSECTOR)
#define REG_LBAL(dev) REG_CMD_BASE(dev, ATA_LBAL)
#define REG_LBAM(dev) REG_CMD_BASE(dev, ATA_LBAM)
#define REG_LBAH(dev) REG_CMD_BASE(dev, ATA_LBAH)
#define REG_DEVICE(dev) REG_CMD_BASE(dev, ATA_DEVICE)
#define REG_STATUS(dev) REG_CMD_BASE(dev, ATA_STATUS)
#define REG_FEATURES(dev) REG_CMD_BASE(dev, ATA_FEATURES)

#define REG_CMD(dev) REG_CMD_BASE(dev, ATA_CMD)
#define REG_CTL(dev) REG_CTL_BASE(dev, ATA_CTL)

#define ATA_IDENT_DEVTYPE 0
#define ATA_IDENT_CYLINDERS 2
#define ATA_IDENT_HEADS 6
#define ATA_IDENT_SECTORS 12
#define ATA_IDENT_SERIAL 20
#define ATA_IDENT_MODEL 54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID 106
#define ATA_IDENT_MAX_LBA 120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200

#define SECT_SIZE 512
