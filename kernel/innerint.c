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

#include <sched.h>
#include <system.h>

#define DIE_MSG()                                                                                        \
    do {                                                                                                 \
        printk("Unsupport Now...[%s]\n", __FUNCTION__);                                                  \
        printk("EFLAGS:%08x CS:%02x EIP:%08x ERRCODE:%x", regs.eflags, regs.cs, regs.eip, regs.errcode); \
        while (1)                                                                                        \
            ;                                                                                            \
    } while (0);

void doDivideError(pt_regs_t regs) { DIE_MSG(); }
void doDebug(pt_regs_t regs) { DIE_MSG(); }
void doNMI(pt_regs_t regs) { DIE_MSG(); }
void doBreakPoint(pt_regs_t regs) { DIE_MSG(); }
void doOverFlow(pt_regs_t regs) { DIE_MSG(); }
void doBoundsCheck(pt_regs_t regs) { DIE_MSG(); }
void doInvalidOpcode(pt_regs_t regs) { DIE_MSG(); }
void doDeviceNotAvailable(pt_regs_t regs) { DIE_MSG(); }
void doDoubleFault(pt_regs_t regs) { DIE_MSG(); }
void doCoprocSegOverRun(pt_regs_t regs) { DIE_MSG(); }
void doInvalidTss(pt_regs_t regs) { DIE_MSG(); }
void doSegNotPresent(pt_regs_t regs) { DIE_MSG(); }
void doStackFault(pt_regs_t regs) { DIE_MSG(); }
void doGeneralProtection(pt_regs_t regs) { DIE_MSG(); }

void do_no_page(void *);
void do_wp_page(void *);
void doPageFault(pt_regs_t regs) {
#if 0
US RW  P - Description
0  0  0 - Supervisory process tried to read a non-present page entry
0  0  1 - Supervisory process tried to read a page and caused a protection fault
0  1  0 - Supervisory process tried to write to a non-present page entry
0  1  1 - Supervisory process tried to write a page and caused a protection fault
1  0  0 - User process tried to read a non-present page entry
1  0  1 - User process tried to read a page and caused a protection fault
1  1  0 - User process tried to write to a non-present page entry
1  1  1 - User process tried to write a page and caused a protection fault
#endif
    // DIE_MSG();
    void *addr;
    u32 errcode = regs.errcode;

    asm("movl %%cr2,%%eax" : "=a"(addr));

    // printk("do page fault errcode %x addr %08x [%08x]\n", errcode, addr, current);

    // assert(errcode != 2 && errcode != 6);

    if ((errcode & PAGE_P) == 0) {
        do_no_page(addr);
    } else {
        do_wp_page(addr);
    }
}

void doCoprocError(pt_regs_t regs) { DIE_MSG(); }
