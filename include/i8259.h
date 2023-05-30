/*
 *--------------------------------------------------------------------------
 *   File Name: i8259.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sun Nov  9 11:37:09 2008
 *     Version: 1.1
 * Last Update: Tue Feb 10 20:28:47 2009
 *
 *--------------------------------------------------------------------------
 */

#ifndef _I8259_H
#define _I8259_H

#include "io.h"
#include "irq.h"

#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_IMR 0x21
#define PIC_SLAVE_CMD 0xA0
#define PIC_SLAVE_IMR 0xA1

#define PIC_MASTER_ISR PIC_MASTER_CMD
#define PIC_SLAVE_ISR PIC_SLAVE_CMD

#define PIC_CASCADE_IR 0x2  // The IR2 on Master Connect to Slave.

#define PIC_AEOI 0

extern void init_i8259();
extern void mask_i8259();

int enable_i8259_irq(unsigned int irq);
int disable_i8259_irq(unsigned int irq);
void mask_ack_i8259_irq(unsigned int irq);
void ack_i8259_irq(unsigned int irq);

#if 0
=Programmable Interrupt Controller=

*Registers:*
	* Interrupt Mask Register		(IMR)
        The IMR specifies which interrupts are to be ignored and not acknowledged. if set the bit.
	* Interrupt Request Register	(IRR)
        The IRR specifies which interrupts are pending acknowledgement, and is typically a symbolic register which can not be directly accessed.
	* In Sevice Register			(ISR)
        The ISR register specifies which interrupts have been acknowledged, but are still waiting for an End Of Interrupt (EOI).
*Port:*
	*Master:*
	* 0x20: W ICW1. W OCW2. W OCW3. R IRR. R ISR.
	* 0x21: W ICW2. W ICW3. W ICW4. RW ICW3. RW ICW4. RW IMR
	*Slave:*
	* 0xA0: W ICW1. W OCW2. W OCW3. R IRR. R ISR.
	* 0xA1: W ICW2. W ICW3. W ICW4. RW IMR.

*ICW1*

_PORT 0x20 : 0xA0_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   |   |
  |   |   |   |   |   |   |   +--[1 Need ICW4 : 0 Not]
  |   |   |   |   |   |   +------[1 Single PIC : 0 Multi]
  |   |   |   |   |   +----------[1 4 Bytes Interrupt Vector : 0 8]
  |   |   |   |   +--------------[1 Level Triggered Mode : 0 Edge]
  |   |   |   +------------------[ 1 == ICW1 ]
  +---+---+----------------------[ 0 == PC System]
}}}

*ICW2*

_PORT 0x21 : 0xA1_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   |   |
  |   |   |   |   |   +---+---+--[000 == 80x86 System]
  +---+---+---+---+--------------[Bit 7..3 80x86 Interrupt Vector]
}}}

*ICW3 For Master*

_PORT 0x21 : 0xA1_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   |   |
  |   |   |   |   |   |   |   +--[1 IRQ0 Connect Slave : 0 No]
  |   |   |   |   |   |   +------[1 IRQ1 Connect Slave : 0 No]
  |   |   |   |   |   +----------[1 IRQ2 Connect Slave : 0 No]
  |   |   |   |   +--------------[1 IRQ3 Connect Slave : 0 No]
  |   |   |   +------------------[1 IRQ4 Connect Slave : 0 No]
  |   |   +----------------------[1 IRQ5 Connect Slave : 0 No]
  |   +--------------------------[1 IRQ6 Connect Slave : 0 No]
  +------------------------------[1 IRQ7 Connect Slave : 0 No]
}}}

*ICW3 For Slaver*

_PORT 0x21 : 0xA1_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   |   |
  |   |   |   |   |   +---+---+--[The Master IR No. Slaver Connect To]
  +---+---+---+---+--------------[00000]
}}}

*ICW4*

_PORT 0x21 : 0xA1_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   |   |
  |   |   |   |   |   |   |   +--[1 80x86 Mode : 0 MCS 80/85]
  |   |   |   |   |   |   +------[1 Auto EOI : 0 Normal EOI]
  |   |   |   |   +---+----------[0X Non - Buffered Mode  :
  |   |   |   |                   11 Master Buffered Mode : 10 Slave]
  |   |   |   +------------------[1 Special Fully Nested Mode : 0 Not]
  +---+---+----------------------[000 Not Used]
}}}

*OCW1*

_PORT 0x21 : 0xA1_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   |   |
  |   |   |   |   |   |   |   +--[0 IRQ0 Open : Close]
  |   |   |   |   |   |   +------[0 IRQ1 Open : Close]
  |   |   |   |   |   +----------[0 IRQ2 Open : Close]
  |   |   |   |   +--------------[0 IRQ3 Open : Close]
  |   |   |   +------------------[0 IRQ4 Open : Close]
  |   |   +----------------------[0 IRQ5 Open : Close]
  |   +--------------------------[0 IRQ6 Open : Close]
  +------------------------------[0 IRQ7 Open : Close]
}}}

*OCW2*

_PORT 0x20 : 0xA0_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 |EOI| 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   +---+---+
  |   |   |   |   |       |      +--[000 Act On IRQ0]
  |   |   |   |   |       |      |--[001 Act On IRQ1]
  |   |   |   |   |       |      |--[010 Act On IRQ2]
  |   |   |   |   |       |_____ |--[011 Act On IRQ3]
  |   |   |   |   |              |--[100 Act On IRQ4]
  |   |   |   +---+---[00]       |--[101 Act On IRQ5]
  |   |   |                      |--[110 Act On IRQ6]
  +---+---+                      +--[111 Act On IRQ7]
      |       +--[000 Rotate In Auto EOI Mode (Clear)]
      |       |--[001 Non Specific EOI]]
      |       |--[010 Reserved]
      |_______|--[011 Specific EOI]
              |--[100 Rotate In Auto EOI Mode (Set)]
              |--[101 Rotate On Non-Specific EOI]
              |--[110 Set Priority Command (Use Bits 2:0)]
              +--[111 Rotate On Specific EOI (Use Bits 2:0)]
}}}

*OCW3*

_PORT 0x20 : 0xA0_
{{{
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
  |   |   |   |   |   |   +---+
  |   |   |   |   |   |     |    +--[00 Reserved]
  |   |   |   |   |   |     |____|--[01 Reserved]
  |   |   |   |   |   |          |--[10 Next Read Returns IRR]
  |   |   |   |   |   |          +--[11 Next Read Returns ISR]
  |   |   |   |   |   +-------------[1 Poll Command : 0 No Poll Command]
  |   |   |   |   +-----------------[1]
  |   +---+   +---------------------[0]
  |     |     +--[00 Reserved]
  |     |_____|--[01 Reserved]
  |           |--[10 Reset Special Mask]
  |           +--[11 Set Special Mask]
  +--[0]
}}}
#endif

#endif  //_I8259_H
