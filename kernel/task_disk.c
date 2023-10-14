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

void disk_init() {
    // ...
}

void send_disk_request(disk_request_t *r) {
    if (NULL == r) {
        panic("null disk request");
    }

    ide_drive_t *drv = ide_get_drive(r->dev);

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

    mutex_lock(&drv->ide_pci_controller->request_mutex);
    atomic_inc(&drv->ide_pci_controller->request_cnt);
    list_add_tail(&r->list, &drv->ide_pci_controller->request_queue.list);
    mutex_unlock(&drv->ide_pci_controller->request_mutex);

    // 唤醒task_disk
    up(&drv->ide_pci_controller->request_queue.sem);

    // 等待被task_disk唤醒
    down(&r->sem);
}

void disk_task_entry(void *arg) {
    int r_cnt = 0;
    while (1) {
        int channel = (int)arg;
        ide_pci_controller_t *ide_ctrl = ide_pci_controller + channel;

        // 为了在DEBUG时看到RUNNING
        int cnt = 2;
        for (int i = 0; i < cnt; i++) {
            asm("hlt;");
        }

        // printk("wait request for hard disk channel %d\n", channel);
        down(&ide_ctrl->request_queue.sem);
        // printk("hard disk channel %d\n", channel);

        mutex_lock(&ide_ctrl->request_mutex);
        disk_request_t *r;
        r = list_first_entry(&ide_ctrl->request_queue.list, disk_request_t, list);
        if (NULL == r) {
            panic("no disk request");
        }

        list_del(&r->list);
        atomic_inc(&ide_ctrl->consumed_cnt);
        mutex_unlock(&ide_ctrl->request_mutex);

        ide_drive_t *drv = ide_get_drive(r->dev);
        int drv_no = drv->drv_no;
        if (drv->present == 0) {
            panic("disk not present");
        }

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
            panic("invalid disk request command");
            break;
        }

        // 等待硬盘中断
        // printk("down ide req\n");
        down(&ide_ctrl->disk_intr_sem);

        // 读数据
        if (DISK_REQ_IDENTIFY == r->command) {
            void ata_read_data(int drv_no, int sect_cnt, void *dst);
            ata_read_data(drv_no, 1, r->buf);
        }

        // 唤醒等待该请求的进程
        up(&(r->sem));
    }
}
