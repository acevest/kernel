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

#ifndef    _PCI_H
#define _PCI_H

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
 * |               Class Code                | Revision    |
 * +-------------+-------------+-------------+-------------+ 0CH
 * |    BIST     | Header Type |LatencyTimer |CacheLineSize|
 * +-------------+-------------+-------------+-------------+ 10H
 * |                Base Address Register 1                |
 * +-------------------------------------------------------+ 14H
 * |                Base Address Register 2                |
 * +-------------------------------------------------------+ 18H
 * |                Base Address Register 3                |
 * +-------------------------------------------------------+ 1CH
 * |                Base Address Register 4                |
 * +-------------------------------------------------------+ 20H
 * |                Base Address Register 5                |
 * +-------------------------------------------------------+ 24H
 * |                Base Address Register 6                |
 * +-------------------------------------------------------+ 28H
 * |                  CardBus CIS Pointer                  |
 * +---------------------------+---------------------------+ 2CH 
 * |        System ID          |        Subsystem ID       |
 * +---------------------------+---------------------------+ 30H
 * |               Expansion ROM Base Address              |
 * +-----------------------------------------+-------------+ 34H
 * |/////////////////////////////////////////|Capabilities |
 * +-----------------------------------------+-------------+ 38H
 * |///////////////////////////////////////////////////////|
 * +-------------+------------+-------------+--------------+ 3CH
 * |   Max_Lat   |   Min_Gnt  |Interrupt PIN|Interrupt Line|
 * +-------------+------------+-------------+--------------+ 40H
 */


#define PCI_VENDORID        0x00
#define PCI_DEVICEID        0x02
#define PCI_COMMAND        0x04
#define PCI_STATUS        0x06
#define PCI_REVISION        0x08
#define PCI_CLASSPROG        0x09
#define PCI_CLASSDEVICE        0x10
#define PCI_HDRTYPE        0x0E
    #define    PCI_HDRTYPE_MASK    0x7F
    #define    PCI_HDRTYPE_NORMAL    0x00
    #define    PCI_HDRTYPE_BRIDGE    0x01
    #define    PCI_HDRTYPE_CARDBUS    0x02
#define PCI_INTRLINE        0x3C
#define PCI_INTRPIN        0x3D
#define PCI_MINGNT        0x3E
#define PCI_MAXLAT        0x3F


#define PCI_CMD(bus, dev, devfn, reg) \
(0x80000000 | (dev << 11) | (devfn << 8) | (reg & 0xFC))

#define PCI_CONFIG_CMD(cmd) (cmd & ~3)
#define PCI_GET_CMD_REG(cmd) (cmd & 0xFF)


/*   PCI IDS   */
// Display
#define PCI_BASE_CLASS_DISPLAY        0x03
#define PCI_CLASS_DISPLAY_VGA        0x0300
// Bridge
#define PCI_BASE_CLASS_BRIDGE        0x06
#define PCI_CLASS_BRIDGE_HOST        0x0600
#define PCI_CLASS_BRIDGE_ISA        0x0601
#define PCI_CLASS_BRIDGE_PCI        0x0604
#define PCI_CLASS_BRIDGE_CARDBUS    0x0607

/* Vendors*/
#define PCI_VENDORID_COMPAQ    0x0E11
#define PCI_VENDORID_INTEL    0x8086
#define PCI_VENDORID_ATI    0x1002
#define PCI_VENDORID_IBM    0x1014
#define PCI_VENDORID_AMD    0x1022
#define PCI_VENDORID_HP        0x103C
#define PCI_VENDORID_SONY    0x104D
#define PCI_VENDORID_MOTOROLA    0x1057
#define PCI_VENDORID_APPLE    0x106B
#define PCI_VENDORID_SUN    0x108E
#define PCI_VENDORID_NVIDIA    0x10DE
#define PCI_VENDORID_REALTEK    0x10EC


// PCI Bridge
// TODO
/*
 * 31                        16 15                         0
 * +---------------------------+---------------------------+ 00H
 * |         Device ID         |          Vendor ID        |
 * +---------------------------+---------------------------+ 04H
 * |           Status          |          Command          |
 * +-----------------------------------------+-------------+ 08H
 * |               Class Code                | Revision    |
 * +-------------+-------------+-------------+-------------+ 0CH
 * |    BIST     | Header Type |LatencyTimer |CacheLineSize|
 * +-------------+-------------+-------------+-------------+ 10H
 * |                Base Address Register 1                |
 * +-------------------------------------------------------+ 14H
 * |                Base Address Register 2                |
 * +-------------------------------------------------------+ 18H
 * |                Base Address Register 3                |
 * +-------------------------------------------------------+ 1CH
 * |                Base Address Register 4                |
 * +-------------------------------------------------------+ 20H
 * |                Base Address Register 5                |
 * +-------------------------------------------------------+ 24H
 * |                Base Address Register 6                |
 * +-------------------------------------------------------+ 28H
 * |                  CardBus CIS Pointer                  |
 * +---------------------------+---------------------------+ 2CH 
 * |        System ID          |        Subsystem ID       |
 * +---------------------------+---------------------------+ 30H
 * |               Expansion ROM Base Address              |
 * +-----------------------------------------+-------------+ 34H
 * |/////////////////////////////////////////|Capabilities |
 * +-----------------------------------------+-------------+ 38H
 * |///////////////////////////////////////////////////////|
 * +-------------+------------+-------------+--------------+ 3CH
 * |   Max_Lat   |   Min_Gnt  |Interrupt PIN|Interrupt Line|
 * +-------------+------------+-------------+--------------+ 40H
 */



#endif //_PCI_H
