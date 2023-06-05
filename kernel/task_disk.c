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
#include <ide.h>
#include <sched.h>

// task disk与中断函数之间的信号量
semaphore_t disk_intr_sem;

// task disk 之间发送命令的互斥量
DECLARE_MUTEX(disk_cmd_mutex);

void disk_init() { semaphore_init(&disk_intr_sem, 0); }

volatile uint32_t disk_request_cnt = 0;
volatile uint32_t disk_handled_cnt = 0;

void send_disk_request(disk_request_t *r) {
    if (NULL == r) {
        panic("null disk request");
    }

    // TODO: 转换
    int drv_no = r->dev;
    // assert(drv_no == 0);

    assert(drv_no >= 0);
    assert(drv_no <= MAX_IDE_DRIVE_CNT);

    ide_drive_t *drv = ide_drives + drv_no;

    assert(drv->present == 1);

    // 这个用来让task_disk唤醒自己
    semaphore_init(&r->sem, 0);

    // 校验pos，和pos+count是否大于硬盘返回的最大LBA48
    // ...

    // 校验buffer是否跨64K
    // 先不处理
    if (((uint32_t)r->buf & 0xFFFF0000) != (((uint32_t)(r->buf + r->count * 512)) & 0xFFFF0000)) {
        panic("disk DMA read cross 64K");
    }

    mutex_lock(&drv->request_mutex);
    disk_request_cnt++;
    list_add_tail(&r->list, &drv->request_queue.list);
    mutex_unlock(&drv->request_mutex);

    // 唤醒task_disk
    up(&drv->request_queue.sem);

    // 等待被task_disk唤醒
    down(&r->sem);
}

void disk_task_entry(int arg) {
    int r_cnt = 0;
    while (1) {
        // 如果要改造成每个drive对应一个内核任务的话
        // 就要注意共享disk_intr_sem的问题
        // 目前只支持drv_no == 0
        int drv_no = arg;
        ide_drive_t *drv = ide_drives + drv_no;
        if (drv->present == 0) {
            panic("disk not present");
        }

        // 为了在DEBUG时看到RUNNING
        for (int i = 0; i < 3; i++) {
            asm("hlt;");
        }

        // printk("wait request for hard disk %d\n", drv_no);
        down(&drv->request_queue.sem);
        // printk("hard disk %d\n", drv_no);

        mutex_lock(&drv->request_mutex);
        disk_request_t *r;
        r = list_first_entry(&drv->request_queue.list, disk_request_t, list);
        if (NULL == r) {
            panic("no disk request");
        }

        list_del(&r->list);
        disk_handled_cnt++;
        mutex_unlock(&drv->request_mutex);

        // 目前这个disk_cmd_mutex是用于两个通道四个DRIVE之间互斥
        // 目前还不确定两个通道之间能否同时执行
        // 后续要把disk分成两个进程,分别负责channel 0 1
        // 这里再视情况改写
        mutex_lock(&disk_cmd_mutex);
        switch (r->command) {
        case DISK_REQ_IDENTIFY:
            assert(r->count == 1);
            void ata_read_identify(int drv, int disable_intr);
            ata_read_identify(drv_no, 0);
            break;
        case DISK_REQ_READ:
            assert(r->count > 0);
            assert(r->buf != NULL);
            void ata_dma_read_ext(int drv, uint64_t pos, uint16_t count, void *dest);
            ata_dma_read_ext(drv_no, r->pos, r->count, r->buf);
            break;
        default:
            break;
        }

        // 等待硬盘中断
        down(&disk_intr_sem);
        mutex_unlock(&disk_cmd_mutex);
        // 读数据
        if (DISK_REQ_IDENTIFY == r->command) {
            void ata_read_data(int drv_no, int sect_cnt, void *dst);
            ata_read_data(drv_no, 1, r->buf);
        }

        // 唤醒等待该请求的进程
        up(&(r->sem));
    }
}
