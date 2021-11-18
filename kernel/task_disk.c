/*
 * ------------------------------------------------------------------------
 *   File Name: task_disk.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:19:00 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <sched.h>
#include <wait.h>
#if 0

typedef enum {
    DISK_REQ_IDENTIFY,
    DISK_REQ_READ,
} disk_request_cmd_t;

typedef struct disk_request {
    uint64_t pos;                // 扇区号
    uint16_t count;              // 扇区数
    void *buf;                   // 到的缓冲区
    disk_request_cmd_t command;  // 命令
    wait_queue_head_t wait;      // 等待队列
    // 驱动器完全有可能在进程在进程睡眠到等待队列前返回数据并执行唤醒操作
    // 这时等待队列上无进程，就相当于不执行任何操作
    // 然后进程再睡眠到等待队列上，就会造成永远无法唤醒该进程
    // 因此要添加一个字段，标志驱动器已经对该请求做过唤醒操作
    // 进程在睡眠前需要检查该字段
    int done;
} disk_request_t;

void send_disk_request() {
    disk_request_t r;
    r.pos = 0;
    r.count = 1;
    r.buf = kmalloc(512, 0);
    r.command = DISK_REQ_IDENTIFY;
    INIT_LIST_HEAD(&r.wait.task_list);
    r.done = 0;

    list_add_tail(&wq->task_list, &head->task_list);

    // 发送命令
    //....

    unsigned long flags;
    irq_save(flags);
    if (0 == r.done) {  // 驱动器还没完成
        set_current_state(TASK_WAIT);
        irq_restore(flags);
        // 就算在schedule前驱动器触发中断也没有问题
        // 因为该进程已经加到等待队列上了
        // 所以它一定以唤醒该进程
        schedule();
    } else {  // 驱动器已经完成
        irq_restore(flags);
    }
}
#endif

void disk_task_entry() {
    while (1) {
        // TODO
        asm("hlt;");
        // schedule();
    }
}
