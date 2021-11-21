/*
 * ------------------------------------------------------------------------
 *   File Name: task_disk.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:19:00 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <disk.h>
#include <sched.h>

disk_request_queue_t disk_request_queue;  // = {.count = 0,
                                          //   .sem = SEMAPHORE_INITIALIZER(disk_request_queue.sem, 0),
                                          //   .list = LIST_HEAD_INIT(disk_request_queue.list)};

void disk_init() {
    disk_request_queue.count = 0;
    disk_request_queue.sem.cnt = 0;
    INIT_LIST_HEAD(&disk_request_queue.sem.wait_list);
    INIT_LIST_HEAD(&disk_request_queue.list);
}

#if 1
void send_disk_request(disk_request_t *r) {
    if (NULL == r) {
        panic("null disk request");
    }

    // 校验pos，和pos+count是否大于硬盘返回的最大LBA48
    // ...

    // 校验buffer是否跨64K
    // 先不处理
    if (((uint32_t)r->buf & 0xFFFF0000) != (((uint32_t)(r->buf + r->count * 512)) & 0xFFFF0000)) {
        panic("disk DMA read cross 64K");
    }

    INIT_LIST_HEAD(&r->wait.task_list);
    r->done = 0;
    // r.pos = pos;
    // r.count = count;
    // r.buf = kmalloc(512, 0);
    // r.command = DISK_REQ_IDENTIFY;
    // INIT_LIST_HEAD(&r.wait.task_list);
    // r.done = 0;

    // printk("do send disk request: %d %x %x %x\n", list_empty(&disk_request_queue.sem.wait_list),
    //        &disk_request_queue.sem.wait_list, disk_request_queue.sem.wait_list.next,
    //        disk_request_queue.sem.wait_list.prev);

    // 发送命令
    unsigned long flags;
    irq_save(flags);
    list_add_tail(&r->list, &disk_request_queue.list);
    irq_restore(flags);

    // 唤醒task_disk
    printk("up sem\n");
    up(&disk_request_queue.sem);

    // 等待task_dist结束
    printk("wait event\n");
    wait_event(&r->wait, r->done != 0);

    printk("wait finished\n");
}
#endif

void disk_task_entry() {
    while (1) {
        void prepare_to_wait_on_ide();
        prepare_to_wait_on_ide();

        printk("wait for new hard disk request\n");
        down(&disk_request_queue.sem);
        printk("hard disk request: %d\n", disk_request_queue.count++);

        unsigned long flags;
        irq_save(flags);

        disk_request_t *r;
        if (list_empty(&disk_request_queue.list)) {
        } else {
            r = list_first_entry(&disk_request_queue.list, disk_request_t, list);
            if (NULL == r) {
                panic("no disk request");
            }

            printk("disk request: pos %ld count %d cmd %d\n", r->pos, r->count, r->command);
        }

        irq_restore(flags);

        if (NULL == r) {
            continue;
        }

        int dev = 0;
        switch (r->command) {
        case DISK_REQ_IDENTIFY:
            assert(r->count == 1);
            void ata_read_identify(int dev);
            ata_read_identify(dev);
            break;
        case DISK_REQ_READ:
            assert(r->count > 0);
            assert(r->buf != NULL);
            void ata_dma_read_ext(int dev, uint64_t pos, uint16_t count, void *dest);
            ata_dma_read_ext(dev, r->pos, r->count, r->buf);
            break;
        default:
            break;
        }

        // 等待硬盘中断
        void wait_on_ide();
        wait_on_ide();

        // 读数据
        if (DISK_REQ_IDENTIFY == r->command) {
            void ata_read_data(int dev, int sect_cnt, void *dst);
            ata_read_data(dev, 1, r->buf);
        }

        // 唤醒等待该请求的进程
        r->done = 1;
        wake_up(&r->wait);
    }
}
