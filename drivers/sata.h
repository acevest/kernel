/*
 * ------------------------------------------------------------------------
 *   File Name: sata.h
 *      Author: Zhao Yanbai
 *              2026-01-16 22:10:01 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <ahci.h>
#include <types.h>

#define SATA_SIGNATURE_ATA 0x00000101
#define SATA_SIGNATURE_ATAPI 0xEB140101
#define SATA_SIGNATURE_SEMB 0xC33C0101
#define SATA_SIGNATURE_PM 0x96690101

#define MAX_SATA_DEVICES 4

#define SATA_CMD_READ_PIO 0x20
#define SATA_CMD_READ_PIO_EXT 0x24
#define SATA_CMD_READ_DMA_EXT 0x25
#define SATA_CMD_READ_DMA 0xC8
#define SATA_CMD_IDENTIFY 0xEC

// 参考ATA的DEVICE的定义
// DEVICE寄存器
// bit7: Obsolete Always set.
// bit6: L 如果为1，LBA Mode
// bit5: Obsolete Always set.
// bit4: DRIVE
// bit[0, 3] HS 如果L为0就是磁头号Head Number，如果L为1，则为LBA28的24-27位
// 虽然理论上每个SATA端口复用器（PM）可以连接多个设备，但不计划支持，所以DRIVE位为0
// 另外LBA在FIS中填写，所以bit0~3都可为0
// 最后：在现代AHCI单设备场景中，这个字段通常被设为0，控制器会自动处理。大多数驱动设为0即可。
// 所以这里定义成0x00 或 0xE0 都行
#define SATA_DEVICE_LBA ((1 << 7) | (1 << 6) | (1 << 5))

typedef struct {
    ahci_hba_t* hba;
    ahci_port_t* port;
    int index;

    int dma;    // 是否支持dma
    int lba48;  // 是否支持lba48
    uint64_t max_lba;

    // 虽然有32个cmd header，但只用第0个
    ahci_cmd_header_t* cmd_list_base_vaddr;
    vaddr_t fis_base_vaddr;

    // 分配一个cmd table页，但只用第0个cmd table
    paddr_t cmd_table_paddr;
    ahci_cmd_table_t* cmd_table_vaddr;

    // 分配一个prdt页，但只用第0个prdt entry
    paddr_t prdte_paddr;
    vaddr_t prdte_vaddr;

    // 分配一个数据页
    paddr_t data_paddr;
    vaddr_t data_vaddr;

    // 第0个cmd header
    ahci_cmd_header_t* cmd_list0;
    // 第0个cmd table
    ahci_cmd_table_t* cmd_table0;
    // 第0个prdte
    ahci_prdt_entry_t* prdte0;
} sata_device_t;

extern sata_device_t sata_devices[MAX_SATA_DEVICES];

void sata_read_identify_string(const uint16_t* identify, int bgn, int end, char* buf);
