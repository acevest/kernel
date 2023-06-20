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

    void ide_read_partions();
    ide_read_partions();

    void ata_read_ext2_sb();
    ata_read_ext2_sb();

    while (1) {
        asm("sti;hlt;");
        sysc_wait(2);
    }
}
