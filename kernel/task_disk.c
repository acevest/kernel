/*
 * ------------------------------------------------------------------------
 *   File Name: task_disk.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:19:00 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <sched.h>

void disk_task_entry() {
    while (1) {
	// TODO
	asm("hlt;");
        //schedule();
    }
}
