/*
 * ------------------------------------------------------------------------
 *   File Name: task_disk.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:19:00 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <ata.h>
#include <completion.h>
#include <disk.h>
#include <ide.h>
#include <sched.h>

void ata_read_identify(int drv, int disable_intr);
void ata_pio_read_data(int drv_no, int sect_cnt, void *dst);
void ata_dma_read_ext(int drv, uint64_t pos, uint16_t count, void *dest);
int ata_pio_read_ext(int drv, uint64_t pos, uint16_t count);

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

        // 为了在DEBUG时看到RUN
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
        assert(drv->present == 1);

        int part_id = DEV_MINOR((r->dev)) & 0xFF;
        assert(part_id < MAX_DISK_PARTIONS);
        assert(MAKE_DISK_DEV(drv_no, part_id) == r->dev);

        uint64_t pos = r->pos + drv->partions[part_id].lba_start;
        assert(pos < drv->partions[part_id].lba_end);

        // 对于DMA的方式来说，一次就能搞定
        uint16_t send_cmd_times = 1;
#if !DISK_DMA_MODE
        // 对于PIO的方式来说，一次只能操作一个扇区，所以有几个扇区就要重试几次
        send_cmd_times = r->count;
        send_cmd_times = 1;
#endif
        for (int count = 0; count < send_cmd_times; count++) {
            switch (r->command) {
            case DISK_REQ_IDENTIFY:
                printk("try to read disk drive %u identify", drv_no);
                assert(r->count == 1);
                ata_read_identify(drv_no, 0);
                break;
            case DISK_REQ_READ:
                assert(r->count > 0);
                assert(r->buf != NULL || r->bb->data != NULL);
#if !DISK_DMA_MODE
                if (r->bb != 0) {
                    ata_pio_read_ext(drv_no, pos + count, r->count);
                } else {
                    ata_pio_read_ext(drv_no, pos + count, r->count);
                }
#else
                if (r->bb != 0) {
                    ata_dma_read_ext(drv_no, pos, r->count, r->bb->data);
                } else {
                    ata_dma_read_ext(drv_no, pos, r->count, r->buf);
                }
#endif
                break;
            default:
                panic("invalid disk request command");
                break;
            }

            // 等待硬盘中断
            // printk("down ide req\n");
            down(&ide_ctrl->disk_intr_sem);

#if !DISK_DMA_MODE
            // void ata_wait(int drv, int timeout);
            // ata_wait(drv_no, 100);
            if (DISK_REQ_READ == r->command || DISK_REQ_IDENTIFY == r->command) {
                uint32_t offset = SECT_SIZE * count;
                if (r->bb != 0) {
                    ata_pio_read_data(drv_no, r->count, r->bb->data + offset);
                } else {
                    ata_pio_read_data(drv_no, r->count, r->buf + offset);
                }
            }

#endif
        }

        if (r->bb != 0) {
            r->bb->uptodate = 1;
            complete(&r->bb->io_done);
        }

        // 唤醒等待该请求的进程
        up(&(r->sem));
    }
}
