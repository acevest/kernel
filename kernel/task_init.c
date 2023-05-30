#include <fcntl.h>
#include <io.h>
#include <irq.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <stat.h>
#include <stdio.h>
#include <syscall.h>
#include <system.h>
#include <types.h>
int sysc_wait(uint32_t ticks);
void init_task_entry() {
    current->priority = 10;

    // 继续内核未完成的初始化
    // 这些初始化在开中断的情况下完成
    void setup_under_irq();
    setup_under_irq();

    while (1) {
        asm("sti;hlt;");
        sysc_wait(2);
    }
}
