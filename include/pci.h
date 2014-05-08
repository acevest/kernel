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

#define PCI_ADDR    0xCF8   // CONFIG_ADDRESS
#define PCI_DATA    0xCFC   // CONFIG_DATA

// PCI Device
/*
 * 31                        16 15                         0
 * +---------------------------+---------------------------+ 00H
 * |         Device ID         |          Vendor ID        |
 * +---------------------------+---------------------------+ 04H
 * |           Status          |          Command          |
 * +-----------------------------------------+-------------+ 08H
 * |  Class Code |  Subclass   |   Prog IF   |  Revision   |
 * +-------------+-------------+-------------+-------------+ 0CH
 * |    BIST     | Header Type |LatencyTimer |CacheLineSize|
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

typedef struct pci_device
{
    list_head_t   list;
    unsigned long vendor;
    unsigned long device;
    unsigned long command;
    unsigned long status;
    unsigned long revision;
    unsigned long classcode;
    unsigned long hdr_type;
    unsigned long bar0, bar1, bar2, bar3, bar4, bar5;
    unsigned long sub_system_id;
    unsigned long system_id;
    unsigned long intr_line;
    unsigned long intr_pin;

    unsigned long primary_bus_nr;   /* only for pci bridge */
} __attribute__((packed)) pci_device_t;


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

#define PCI_VENDORID        0x00
#define PCI_DEVICEID        0x02
#define PCI_COMMAND         0x04
#define PCI_STATUS          0x06
#define PCI_REVISION        0x08
#define PCI_PROGIF          0x09
#define PCI_CLASSCODE       0x0A
#define PCI_HDRTYPE         0x0E
    #define    PCI_HDRTYPE_MASK     0x7F
    #define    PCI_HDRTYPE_NORMAL   0x00
    #define    PCI_HDRTYPE_BRIDGE   0x01    /* PCI-to-PCI Bridge */
    #define    PCI_HDRTYPE_CARDBUS  0x02    /* CardBus Bridge */
#define PCI_PRIMARY_BUS_NUMBER    0x18
#define PCI_SECONDARY_BUS_NUMBER  0x19
#define PCI_INTRLINE        0x3C
#define PCI_INTRPIN         0x3D
#define PCI_MINGNT          0x3E
#define PCI_MAXLAT          0x3F

// PCI Command Register
#define PCI_CMD(bus, dev, devfn, reg) \
(0x80000000 | (dev << 11) | (devfn << 8) | (reg & 0xFC))

#define PCI_CONFIG_CMD(cmd) (cmd & ~3)
#define PCI_GET_CMD_REG(cmd) (cmd & 0xFF)


/*   PCI IDS   */
// Display
#define PCI_BASE_CLASS_DISPLAY      0x03
#define PCI_CLASS_DISPLAY_VGA       0x0300
// Bridge
#define PCI_BASE_CLASS_BRIDGE       0x06
#define PCI_CLASS_BRIDGE_HOST       0x0600
#define PCI_CLASS_BRIDGE_ISA        0x0601
#define PCI_CLASS_BRIDGE_PCI        0x0604
#define PCI_CLASS_BRIDGE_CARDBUS    0x0607

/* Vendors*/
#define PCI_VENDORID_COMPAQ         0x0E11
#define PCI_VENDORID_INTEL          0x8086
#define PCI_VENDORID_ATI            0x1002
#define PCI_VENDORID_IBM            0x1014
#define PCI_VENDORID_AMD            0x1022
#define PCI_VENDORID_HP             0x103C
#define PCI_VENDORID_SONY           0x104D
#define PCI_VENDORID_MOTOROLA       0x1057
#define PCI_VENDORID_APPLE          0x106B
#define PCI_VENDORID_SUN            0x108E
#define PCI_VENDORID_NVIDIA         0x10DE
#define PCI_VENDORID_REALTEK        0x10EC


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
