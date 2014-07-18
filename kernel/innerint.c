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

void doDivideError(pt_regs_t regs)
{
    DIE_MSG();
}
void doDebug(pt_regs_t regs)
{
    DIE_MSG();
}
void doNMI(pt_regs_t regs)
{
    DIE_MSG();
}
void doBreakPoint(pt_regs_t regs)
{
    DIE_MSG();
}
void doOverFlow(pt_regs_t regs)
{
    DIE_MSG();
}
void doBoundsCheck(pt_regs_t regs)
{
    DIE_MSG();
}
void doInvalidOpcode(pt_regs_t regs)
{
    DIE_MSG();
}
void doDeviceNotAvailable(pt_regs_t regs)
{
    DIE_MSG();
}
void doDoubleFault(pt_regs_t regs)
{
    DIE_MSG();
}
void doCoprocSegOverRun(pt_regs_t regs)
{
    DIE_MSG();
}
void doInvalidTss(pt_regs_t regs)
{
    DIE_MSG();
}
void doSegNotPresent(pt_regs_t regs)
{
    DIE_MSG();
}
void doStackFault(pt_regs_t regs)
{
    DIE_MSG();
}
void doGeneralProtection(pt_regs_t regs)
{
    DIE_MSG();
}
void doPageFault(pt_regs_t regs)
{
    //DIE_MSG();
    void    *addr;
    u32    errcode = regs.errcode;

    asm("movl %%cr2,%%eax":"=a"(addr));

    unsigned long a = (unsigned long) addr;

    a = 0;

    //printd("do page fault errcode %x addr %08x\n", errcode, addr);

    if(errcode & PAGE_US)
    {
        //panic("user program try to access a page and cause a protection fault. addr %08x", addr);
    }

    if((errcode & PAGE_P) == 0)
    {
        extern void do_no_page(void *);
        do_no_page(addr);
    }
    else
    {
        extern void do_wp_page(void *);
        do_wp_page(addr);
    }
}
void doCoprocError(pt_regs_t regs)
{
    DIE_MSG();
}
