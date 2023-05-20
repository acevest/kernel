/*
 * ------------------------------------------------------------------------
 *   File Name: task_disk.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:19:00 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <completion.h>
#include <disk.h>
#include <sched.h>

disk_request_queue_t disk_request_queue;

// task disk与中断函数之间的信号量
semaphore_t disk_intr_sem;

DECLARE_MUTEX(disk_request_mutex);

void disk_init() {
    disk_request_queue.count = 0;
    INIT_LIST_HEAD(&disk_request_queue.list);
    // disk_request_queue.sem.cnt = 0;
    // INIT_LIST_HEAD(&disk_request_queue.sem.wait_list);
    semaphore_init(&disk_request_queue.sem, 0);

    semaphore_init(&disk_intr_sem, 0);
}

void send_disk_request(disk_request_t *r) {
    if (NULL == r) {
        panic("null disk request");
    }

    // 这个用来让task_disk唤醒自己
    semaphore_init(&r->sem, 0);

    // 校验pos，和pos+count是否大于硬盘返回的最大LBA48
    // ...

    // 校验buffer是否跨64K
    // 先不处理
    if (((uint32_t)r->buf & 0xFFFF0000) != (((uint32_t)(r->buf + r->count * 512)) & 0xFFFF0000)) {
        panic("disk DMA read cross 64K");
    }

#if 1
    mutex_lock(&disk_request_mutex);
    list_add_tail(&r->list, &disk_request_queue.list);
    mutex_unlock(&disk_request_mutex);
#else
    // 发送命令
    unsigned long flags;
    irq_save(flags);
    list_add_tail(&r->list, &disk_request_queue.list);
    irq_restore(flags);
#endif

    // 唤醒task_disk
    up(&disk_request_queue.sem);

    // 等待被task_disk唤醒
    down(&r->sem);
}

void disk_task_entry() {
    int r_cnt = 0;
    while (1) {
        // printk("wait for new hard disk request\n");
        down(&disk_request_queue.sem);
        // printk("hard disk request: %d\n", disk_request_queue.count++);

#if 1
        mutex_lock(&disk_request_mutex);
        disk_request_t *r;
        r = list_first_entry(&disk_request_queue.list, disk_request_t, list);
        if (NULL == r) {
            panic("no disk request");
        }

        list_del(&r->list);
        mutex_unlock(&disk_request_mutex);
#else
        unsigned long flags;
        irq_save(flags);

        disk_request_t *r;
        if (list_empty(&disk_request_queue.list)) {
            panic("disk request should not empty");
        }

        r = list_first_entry(&disk_request_queue.list, disk_request_t, list);
        if (NULL == r) {
            panic("no disk request");
        }

        printk("disk request[%d]: dev %d pos %ld count %d cmd %d\n", r_cnt++, r->dev, r->pos, r->count, r->command);
        list_del(&r->list);
        irq_restore(flags);
#endif

        switch (r->command) {
        case DISK_REQ_IDENTIFY:
            assert(r->count == 1);
            void ata_read_identify(int dev, int disable_intr);
            ata_read_identify(r->dev, 0);
            break;
        case DISK_REQ_READ:
            assert(r->count > 0);
            assert(r->buf != NULL);
            void ata_dma_read_ext(int dev, uint64_t pos, uint16_t count, void *dest);
            ata_dma_read_ext(r->dev, r->pos, r->count, r->buf);
            break;
        default:
            break;
        }

        // 等待硬盘中断
        down(&disk_intr_sem);

        // 读数据
        if (DISK_REQ_IDENTIFY == r->command) {
            void ata_read_data(int dev, int sect_cnt, void *dst);
            ata_read_data(r->dev, 1, r->buf);
        }

        // 唤醒等待该请求的进程
        up(&(r->sem));
    }
}
