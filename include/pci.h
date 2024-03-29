/*
 *--------------------------------------------------------------------------
 *   File Name: pci.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sun Mar  8 21:32:16 2009
 * Last Update: Sun Mar  8 21:32:16 2009
 *
 *--------------------------------------------------------------------------
 */

#pragma once

#include <list.h>
#include <types.h>

#define PCI_ADDR 0xCF8  // CONFIG_ADDRESS
#define PCI_DATA 0xCFC  // CONFIG_DATA

// PCI Command
// 这个PCI_CMD是写入PCI_ADDR的，通过bus,dev,fn,reg可以定位到某个PCI总线(可以有多条PCI总线)上的某个设备的某个功能的某个寄存器
// 0 ~ 7: 寄存器偏移
// 8 ~ 10: 功能号
// 11 ~ 15: 设备号
// 16 ~ 23: 总线号
// 24 ~ 30: 保留
// 31: 有效位
#define PCI_CMD(bus, dev, fn, reg) (0x80000000 | (bus << 16) | (dev << 11) | (fn << 8) | reg)
#define PCI_CONFIG_CMD(cmd) (cmd & ~3)
#define PCI_GET_CMD_REG(cmd) (cmd & 0xFF)

// PCI Device
//  All PCI compliant devices must support the Vendor ID, Device ID, Command and Status, Revision ID, Class Code and
//  Header Type fields.Implementation of the other registers is optional, depending upon the devices functionality.

// Command: Provides control over a device's ability to generate and respond to PCI cycles. Where the only functionality
// guaranteed to be supported by all devices is, when a 0 is written to this register, the device is disconnected from
// the PCI bus for all accesses except Configuration Space access. Class Code: A read-only register that specifies the
// type of function the device performs.
// Command的bit2比较重要(Bus Master)，如果将它置为1，则该设备可以充当总线的主设备，否则，设备无法生成对PCI的访问。
// 这个位需要在对硬盘DMA的时候用到

// Subclass: A read-only register that specifies the specific function the device performs.

// Prog IF(Programming Interface Byte): 一个只读寄存器，指定设备具有的寄存器级别的编程接口(如果有的话)

// Header Type: 表示0x10字节开始的具体布局，同时指示该Device是否支持多个Function
//  0x00 - General Device
//  0x01 - PCI-to-PCI Bridge
//  0x02 - CardBus Bridge
//  如果bit7置位，则该Device有多个Function，否则为只支持单个Function的Device

// Interrupt Line: 指示该设备连接到系统中的中断控制器的哪个引脚，对于x86架构来说，该寄存器对应于中断控制器IRQ编号0-15（
// 而不是 I/O APIC IRQ 编号。  值0xFF为没有连接的意思。

// Interrupt Pin: 指明设备使用哪个中断引脚，值0x01对应INTA#, 0x02对应INTB#， 0x03对应INTC#, 0x04对应INTD#,
// 0x00表示不使用中断引脚。

// Capabilities Pointer: Points (i.e. an offset into this function's configuration space) to a linked list of new
// capabilities implemented by the device. Used if bit 4 of the status register (Capabilities List bit) is set to 1. The
// bottom two bits are reserved and should be masked before the Pointer is used to access the Configuration Space.

/*
 * 31                        16 15                         0
 * +---------------------------+---------------------------+ 00H
 * |         Device ID         |          Vendor ID        |
 * +---------------------------+---------------------------+ 04H
 * |           Status          |          Command          |
 * +-----------------------------------------+-------------+ 08H
 * |  Class Code |  Subclass   |   Prog IF   |  Revision   |
 * +-------------+-------------+-------------+-------------+ 0CH
 * |    BIST     | Header Type |LatencyTimer |CacheLineSize|           // BITS: built-in self test
 * +-------------+-------------+-------------+-------------+ 10H
 * |                Base Address Register 0                |
 * +-------------------------------------------------------+ 14H
 * |                Base Address Register 1                |
 * +-------------------------------------------------------+ 18H
 * |                Base Address Register 2                |
 * +-------------------------------------------------------+ 1CH
 * |                Base Address Register 3                |
 * +-------------------------------------------------------+ 20H
 * |                Base Address Register 4                |
 * +-------------------------------------------------------+ 24H
 * |                Base Address Register 5                |
 * +-------------------------------------------------------+ 28H
 * |                  CardBus CIS Pointer                  |
 * +---------------------------+---------------------------+ 2CH
 * |        System ID          |        Subsystem ID       |
 * +---------------------------+---------------------------+ 30H
 * |               Expansion ROM Base Address              |
 * +-----------------------------------------+-------------+ 34H
 * |/////////////////////////////////////////|Capabilities |
 * |/////////////////////////////////////////|Pointer      |
 * +-----------------------------------------+-------------+ 38H
 * |///////////////////////////////////////////////////////|
 * +-------------+------------+-------------+--------------+ 3CH
 * | Max Latency | Min Grant  |Interrupt PIN|Interrupt Line|
 * +-------------+------------+-------------+--------------+ 40H
 */

extern list_head_t pci_devs;

#define BARS_CNT 6

typedef struct pci_device {
    list_head_t list;
    unsigned int bus, dev, devfn;

    unsigned int vendor;
    unsigned int device;
    unsigned int command;
    unsigned int status;
    unsigned int revision;
    unsigned int progif;
    unsigned int classcode;
    unsigned int hdr_type;
    // unsigned int bar0, bar1, bar2, bar3, bar4, bar5;
    unsigned int bars[BARS_CNT];
    unsigned int sub_system_id;
    unsigned int system_id;
    unsigned int intr_line;
    unsigned int intr_pin;

    unsigned int primary_bus_nr; /* only for pci bridge */
    unsigned int secondary_bus_nr;
} pci_device_t;

#if 0
typedef union pci_device
{
    u32_t regs[16];
    struct {
        u16_t   vendor, device;
        u16_t   command, status;
        u32_t   revision : 8, classcode : 24;
        u8_t    cache_line_size, latency_timer, header_type, bist;
        u32_t   bar0;
        u32_t   bar1;
        u32_t   bar2;
        u32_t   bar3;
        u32_t   bar4;
        u32_t   bar5;
        u32_t   card_bus_csi_pointer;
        u16_t   sub_system_id, system_id;
        u32_t   expansion_rom_base_address;
        u64_t   capabilities_pointer : 8, reserved : 56;
        u8_t    intr_line, intr_pin, min_grant, max_latency;
    };
} __attribute__((packed)) pci_device_t;
#endif

#define PCI_VENDORID 0x00
#define PCI_DEVICEID 0x02
#define PCI_COMMAND 0x04
#define PCI_COMMAND_IO 0x01
#define PCI_COMMAND_MEMORY 0x02
#define PCI_COMMAND_MASTER 0x04
#define PCI_COMMAND_SPECIAL 0x08
#define PCI_COMMAND_INVALIDATE 0x10
#define PCI_COMMAND_VGA_PALETTE 0x20
#define PCI_COMMAND_PARITY 0x40
#define PCI_COMMAND_WAIT 0x80
#define PCI_COMMAND_SERR 0x100
#define PCI_COMMAND_FAST_BACK 0x200
#define PCI_COMMAND_INTR_DISABLE 0x400
#define PCI_STATUS 0x06
#define PCI_REVISION 0x08
#define PCI_PROGIF 0x09
#define PCI_CLASSCODE 0x0A
#define PCI_HDRTYPE 0x0E
#define PCI_HDRTYPE_MASK 0x7F
#define PCI_HDRTYPE_NORMAL 0x00
#define PCI_HDRTYPE_BRIDGE 0x01  /* PCI-to-PCI Bridge */
#define PCI_HDRTYPE_CARDBUS 0x02 /* CardBus Bridge */
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_PRIMARY_BUS_NUMBER 0x18
#define PCI_SECONDARY_BUS_NUMBER 0x19
#define PCI_INTRLINE 0x3C
#define PCI_INTRPIN 0x3D
#define PCI_MINGNT 0x3E
#define PCI_MAXLAT 0x3F

/*   PCI IDS   */
// Display
#define PCI_BASE_CLASS_DISPLAY 0x03
#define PCI_CLASS_DISPLAY_VGA 0x0300
// Bridge
#define PCI_BASE_CLASS_BRIDGE 0x06
#define PCI_CLASS_BRIDGE_HOST 0x0600
#define PCI_CLASS_BRIDGE_ISA 0x0601
#define PCI_CLASS_BRIDGE_PCI 0x0604
#define PCI_CLASS_BRIDGE_CARDBUS 0x0607

/* Vendors*/
#define PCI_VENDORID_COMPAQ 0x0E11
#define PCI_VENDORID_INTEL 0x8086
#define PCI_VENDORID_ATI 0x1002
#define PCI_VENDORID_IBM 0x1014
#define PCI_VENDORID_AMD 0x1022
#define PCI_VENDORID_HP 0x103C
#define PCI_VENDORID_SONY 0x104D
#define PCI_VENDORID_MOTOROLA 0x1057
#define PCI_VENDORID_APPLE 0x106B
#define PCI_VENDORID_SUN 0x108E
#define PCI_VENDORID_NVIDIA 0x10DE
#define PCI_VENDORID_REALTEK 0x10EC

pci_device_t *pci_find_device(unsigned int vendor, unsigned int device);
pci_device_t *pci_find_device_by_classcode(unsigned int classcode);

static inline u32 pci_cmd(pci_device_t *pci, unsigned int reg) { return PCI_CMD(pci->bus, pci->dev, pci->devfn, reg); }

int pci_read_config_byte(int cmd);
int pci_read_config_word(int cmd);
int pci_read_config_long(int cmd);
void pci_write_config_byte(int value, int cmd);
void pci_write_config_word(int value, int cmd);
void pci_write_config_long(int value, int cmd);

// PCI Bridge
/*
 * 31                        16 15                         0
 * +---------------------------+---------------------------+ 00H
 * |         Device ID         |          Vendor ID        |
 * +---------------------------+---------------------------+ 04H
 * |           Status          |          Command          |
 * +-------------+---------------------------+-------------+ 08H
 * | Class Code  |  Subclass   |   Prog IF   |  Revision   |
 * +-------------+-------------+-------------+-------------+ 0CH
 * |    BIST     | Header Type |LatencyTimer |CacheLineSize|
 * +-------------+-------------+-------------+-------------+ 10H
 * |                Base Address Register 0                |
 * +-------------------------------------------------------+ 14H
 * |                Base Address Register 1                |
 * +-------------+-------------+-------------+-------------+ 18H
 * |Secondary    | Subordinate |Secondary    |PCI          |
 * |Latency      | Bus         |Bus          |Bus          |
 * |Timer        | Number      |Number       |Number       |
 * +---------------------------+-------------+-------------+ 1CH
 * |    Secondary Status       |  IO Limit   |   IO Base   |
 * +---------------------------+-------------+-------------+ 20H
 * |    Memory Limit           |    Memory Base            |
 * +---------------------------+---------------------------+ 24H
 * | Prefetchable Memory Limit | Prefetchable Memory Base  |
 * +---------------------------+---------------------------+ 28H
 * |           Prefetchable Base  Upper 32bit              |
 * +---------------------------+---------------------------+ 2CH
 * |           Prefetchable Limit Upper 32bit               |
 * +---------------------------+---------------------------+ 30H
 * |   IO Limit Upper 16bit    |  IO Base Upper 16bit      |
 * +-----------------------------------------+-------------+ 34H
 * |/////////////////////////////////////////|Capabilities |
 * |/////////////////////////////////////////|Pointer      |
 * +-----------------------------------------+-------------+ 38H
 * |               Expansion ROM Base Address              |
 * +-------------+------------+-------------+--------------+ 3CH
 * |      Bridge Control      |Interrupt PIN|Interrupt Line|
 * +-------------+------------+-------------+--------------+ 40H
 */

#if 0
typedef union pci_bridge
{
    u32_t regs[16];
    struct {
        u16_t   vendor, device;
        u16_t   command, status;
        u8_t    revision, prog_if, subclass, classcode;
        u8_t    cache_line_size, latency_timer, header_type, bist;
        u32_t   bar0;
        u32_t   bar1;
        u8_t    primary_bus_nr, secondary_bus_nr, subordinate_bus_nr, secondary_latency_timer;
        u32_t   io_base: 8, io_limit : 8, secondary_status : 16;
        u16_t   memory_base, memory_limit;
        u16_t   prefetchable_memory_base, prefetchable_memory_limit;
        u32_t   prefetchable_base_upper;
        u32_t   prefetchable_limit_upper;
        u16_t   io_base_upper, io_limit_upper;
        u32_t   capabilities_pointer : 8, reserved : 24;
        u32_t   expansion_rom_base_address;
        u32_t   intr_lin : 8, intr_pin : 8, bridge_control : 16;
    };
} __attribute__((packed)) pci_bridge_t;
#endif
