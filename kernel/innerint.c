/*
 *--------------------------------------------------------------------------
 *   File Name: innerint.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Mon Nov 10 15:58:46 2008
 * Last Update: Tue Feb 10 22:39:24 2009
 *     Version:    2.0
 * Last Update: Fri Jul 10 11:55:39 2009
 * 
 *--------------------------------------------------------------------------
 */

#include <system.h>
#include <sched.h>

#define DIE_MSG() do{\
    printk("Unsupport Now...[%s]\n", __FUNCTION__);        \
    printk("EFLAGS:%08x CS:%02x EIP:%08x ERRCODE:%x",     \
    regs.eflags, regs.cs, regs.eip, regs.errcode);        \
    while(1);                        \
}while(0);

void doDivideError(PtRegs regs)
{
    DIE_MSG();
}
void doDebug(PtRegs regs)
{
    DIE_MSG();
}
void doNMI(PtRegs regs)
{
    DIE_MSG();
}
void doBreakPoint(PtRegs regs)
{
    DIE_MSG();
}
void doOverFlow(PtRegs regs)
{
    DIE_MSG();
}
void doBoundsCheck(PtRegs regs)
{
    DIE_MSG();
}
void doInvalidOpcode(PtRegs regs)
{
    DIE_MSG();
}
void doDeviceNotAvailable(PtRegs regs)
{
    DIE_MSG();
}
void doDoubleFault(PtRegs regs)
{
    DIE_MSG();
}
void doCoprocSegOverRun(PtRegs regs)
{
    DIE_MSG();
}
void doInvalidTss(PtRegs regs)
{
    DIE_MSG();
}
void doSegNotPresent(PtRegs regs)
{
    DIE_MSG();
}
void doStackFault(PtRegs regs)
{
    DIE_MSG();
}
void doGeneralProtection(PtRegs regs)
{
    DIE_MSG();
}
void doPageFault(PtRegs regs)
{
    //DIE_MSG();
    void    *addr;
    u32    errcode = regs.errcode;

    asm("movl %%cr2,%%eax":"=a"(addr));

/*
    unsigned long cr3;
    asm("movl %%cr3,%%eax":"=a"(cr3));
    printk("%08x errcode: %08x cr2: %08x cr3: %08x\n",
        current, errcode, addr, cr3);
*/

    unsigned long a = (unsigned long) addr;

    a = 0;

    if((errcode & PAGE_P) == 0)
    {
        extern    void    do_no_page(void *);
        do_no_page(addr);
    }
    else
    {
        extern    void    do_wp_page(void *);
        do_wp_page(addr);
    }
}
void doCoprocError(PtRegs regs)
{
    DIE_MSG();
}
