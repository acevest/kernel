/*
 * ------------------------------------------------------------------------
 *   File Name: ahci.h
 *      Author: Zhao Yanbai
 *              2026-01-16 21:32:06 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <types.h>

const static int AHCI_PORT_COUNT = 32;
const static int AHCI_CMD_SLOT_COUNT = 32;

const static int AHCI_DEVICE_NULL = 0;
const static int AHCI_DEVICE_SATA = 1;
const static int AHCI_DEVICE_SEMB = 2;
const static int AHCI_DEVICE_PM = 3;
const static int AHCI_DEVICE_SATAPI = 4;

const static int AHCI_FIS_TYPE_REG_H2D = 0x27;
const static int AHCI_FIS_TYPE_REG_D2H = 0x34;

const static uint32_t AHCI_INTERRUPT_ENABLE_DHRS = (1U << 0);   // device to host fis interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_PSE = (1U << 1);    // pio setup fis interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_DSE = (1U << 2);    // DMA setup fis interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_SDBE = (1U << 3);   // set device bits interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_UFE = (1U << 4);    // unknown fis interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_DPE = (1U << 5);    // descriptor process error interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_PCE = (1U << 6);    // port change error interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_DMPE = (1U << 7);   // device mechanical presence enable
const static uint32_t AHCI_INTERRUPT_ENABLE_PRCE = (1U << 22);  // phyrdy change interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_IPME = (1U << 23);  // incorrect port multiplier interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_OFE = (1U << 24);   // overflow error interrupt enable
const static uint32_t AHCI_INTERRUPT_ENABLE_INFE = (1U << 26);  // interface non-fatal error enable
const static uint32_t AHCI_INTERRUPT_ENABLE_IFE = (1U << 27);   // interface fatal error enable
const static uint32_t AHCI_INTERRUPT_ENABLE_HBDE = (1U << 28);  // host bus data error enable
const static uint32_t AHCI_INTERRUPT_ENABLE_HBFE = (1U << 29);  // host bus fatal error enable
const static uint32_t AHCI_INTERRUPT_ENABLE_TFEE = (1U << 30);  // task file error enable
const static uint32_t AHCI_INTERRUPT_ENABLE_CPDE = (1U << 31);  // cold presence detect enable

typedef struct {
    uint8_t fis_type;
    uint8_t pmport : 4;  // port multiplier
    uint8_t _reserved0 : 3;
    uint8_t c : 1;  // 0: control; 1: command

    uint8_t command;
    uint8_t feature;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;

    uint8_t count_low;
    uint8_t count_high;
    uint8_t icc;
    uint8_t control;

    uint8_t _reserved1[4];
} ahci_fis_reg_h2d_t;

typedef struct {
    uint32_t data_base;  // 第0位必为0

    uint32_t _data_base_upper;

    uint32_t _reserved;

    uint32_t data_byte_count : 22;  // 第0位必为1。另外该值表示实际为data_byte_count+1字节
    uint32_t _reserved1 : 9;
    uint32_t ioc : 1;  // interrupt on completion

} ahci_prdt_entry_t;

typedef struct {
    // 0x00 - 0x40
    uint8_t cmd_fis[64];

    // 0x40 - 0x50
    uint8_t _atapi_cmd[16];  // 12 or 16  bytes

    // 0x50 - 0x80
    uint8_t _reserved[48];

    // 0x80
    ahci_prdt_entry_t prdt[0];  // update to 65536 entries
} ahci_cmd_table_t;

// prd: physical region descriptor
typedef struct {
    uint32_t cfl : 5;  // length of the command fis: 命令FIS大小(双字为单位)
    uint32_t a : 1;    // 该命令需要发送到ATAPI设备
    uint32_t w : 1;    // 该命令需要向设备写入数据
    uint32_t p : 1;    //
    uint32_t r : 1;
    uint32_t b : 1;
    uint32_t c : 1;  // 命令执行完后，需要将 task_file_data的BSY位清零
    uint32_t R : 1;
    uint32_t pmp : 4;
    uint32_t prdtl : 16;  // prdt entry count

    uint32_t prd_byte_count;  // prd byte count

    uint32_t cmd_table_base;  // 指向ahci_cmd_table_t
    uint32_t _cmd_table_base_upper;

    uint32_t reserved[4];
} ahci_cmd_header_t;

typedef struct {
    uint32_t cmd_list_base;
    uint32_t _cmd_list_base_upper;
    uint32_t fis_base;
    uint32_t _fis_base_upper;
    uint32_t interrupt_status;
    uint32_t interrupt_enable;
    uint32_t command_and_status;
    uint32_t reserved;
    uint32_t task_file_data;
    uint32_t signature;
    // DET[03:00]: Device Detection
    // SPD[07:04]: Current Interface Speed
    // IPM[11:08]: Interface Power Management
    uint32_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t cmd_issue;
    uint32_t sata_notification;
    uint32_t fis_base_switch_control;
    uint32_t device_sleep;
    uint32_t reserved2[10];
    uint32_t vendor_specific[4];
} ahci_port_t;

typedef struct {
    // 0x00 - 0x2C
    uint32_t capability;
#define AHCI_ENABLE 0x80000000
#define AHCI_INTERRUPT_ENABLE 0x00000002
#define AHCI_RESET 0x00000001
    uint32_t global_hba_control;
    uint32_t interrupt_status;
    uint32_t ports_implemented;
    uint32_t version;
    uint32_t ccc_ctrl;             // command completion coalescing control
    uint32_t ccc_ports;            // command completion coalescing ports
    uint32_t em_loc;               // enclosure management location
    uint32_t em_ctl;               // enclosure management control
    uint32_t capability_extended;  // capability extended
    uint32_t bios_handoff_control_status;

    // 0x2C - 0xA0
    uint8_t reserved[0xA0 - 0x2C];

    // 0xA0 - 0x100
    uint8_t vendor_specific[0x100 - 0xA0];

    // 0x100
    ahci_port_t ports[32];
} ahci_hba_t;
