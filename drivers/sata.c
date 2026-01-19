/*
 * ------------------------------------------------------------------------
 *   File Name: sata.c
 *      Author: Zhao Yanbai
 *              2025-12-31 20:29:10 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <sata.h>
#include <printk.h>
#include <ioremap.h>
#include <page.h>
#include <assert.h>
#include <string.h>
#include <system.h>

int sata_irq_triggered = 0;

void init_sata_device(ahci_hba_t* hba, ahci_port_t* port, int index) {
    assert(hba != NULL);
    assert(port != NULL);
    assert(index >= 0 && index < MAX_SATA_DEVICES);

    sata_device_t* sata = sata_devices + index;
    sata->hba = hba;
    sata->port = port;
    sata->index = index;

    printk("ahci port clb %08x fb %08x sata_status 0x%08X signature %08X\n", port->cmd_list_base, port->fis_base,
           port->sata_status, port->signature);

    vaddr_t cmd_list_base = (vaddr_t)ioremap(PAGE_ALIGN(port->cmd_list_base), 0x1000);
    vaddr_t fis_base = cmd_list_base;

    if (PAGE_ALIGN(port->cmd_list_base) != PAGE_ALIGN(port->fis_base)) {
        fis_base = (vaddr_t)ioremap(PAGE_ALIGN(port->fis_base), 0x1000);
    }

    cmd_list_base += port->cmd_list_base - PAGE_ALIGN(port->cmd_list_base);
    fis_base += port->fis_base - PAGE_ALIGN(port->fis_base);

    printk(" cmd_list_base %08x->%08x\n", port->cmd_list_base, cmd_list_base);
    printk("      fis_base %08x->%08x\n", port->fis_base, fis_base);

    sata->cmd_list_base_vaddr = (ahci_cmd_header_t*)cmd_list_base;
    sata->fis_base_vaddr = fis_base;

    //
    // 1. 端口重置
    // 2. 分配空间，设置cmd_list_base, fis_base
    // 3. 将sata_error清空
    // 4. 将需要使用的中断通过interrupt_enable开启

    //
    assert(sizeof(ahci_cmd_header_t) == 32);
    ahci_cmd_header_t* cmd_list = (ahci_cmd_header_t*)sata->cmd_list_base_vaddr;
#if 1
    for (int i = 0; i < 32; i++) {
        ahci_cmd_header_t* hdr = &cmd_list[i];
        printk(" cmd_list[%02d] base %08x prdt %04x:%d\n", i, hdr->cmd_table_base, hdr->prdtl, hdr->prdtl);
        hdr->cmd_table_base = 0;
        hdr->prdtl = 0;
    }
#endif

    sata->cmd_table_paddr = (paddr_t)page2pa(alloc_one_page(0));
    sata->cmd_table_vaddr = (ahci_cmd_table_t*)ioremap(sata->cmd_table_paddr, PAGE_SIZE);
    memset((void*)sata->cmd_table_vaddr, 0, PAGE_SIZE);

    sata->data_paddr = (paddr_t)page2pa(alloc_one_page(0));
    sata->data_vaddr = (vaddr_t)ioremap(sata->data_paddr, PAGE_SIZE);
    memset((void*)sata->data_vaddr, 0, PAGE_SIZE);

    memset((void*)cmd_list, 0, sizeof(ahci_cmd_header_t));
    cmd_list[0].cfl = 0;
    cmd_list[0].a = 0;
    cmd_list[0].w = 0;
    cmd_list[0].pmp = 0;
    cmd_list[0].prdtl = 1;
    cmd_list[0].prd_byte_count = 0;
    cmd_list[0].cmd_table_base = sata->cmd_table_paddr;
    sata->cmd_list0 = cmd_list;

    ahci_cmd_table_t* cmd_table = (ahci_cmd_table_t*)sata->cmd_table_vaddr;
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));
    sata->cmd_table0 = cmd_table + 0;
    sata->prdte0 = cmd_table[0].prdt + 0;

    sata->prdte0->data_base = sata->data_paddr;
    sata->prdte0->data_byte_count = (512 - 1) | 1;
    sata->prdte0->ioc = 1;

    //
    ahci_fis_reg_h2d_t* fis = (ahci_fis_reg_h2d_t*)sata->cmd_table0->cmd_fis;
    memset(fis, 0, sizeof(ahci_fis_reg_h2d_t));
    fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = SATA_CMD_IDENTIFY;
    fis->device = SATA_DEVICE_LBA;

    sata->cmd_list0->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    sata->cmd_list0->prdtl = 1;
    sata->cmd_list0->w = 0;  // read
    sata->cmd_list0->a = 0;  // ata device
    sata->cmd_list0->prd_byte_count = 0;

    // 清除中断状态
    // 写1清0
    port->interrupt_status = port->interrupt_status;

    port->interrupt_enable = AHCI_INTERRUPT_ENABLE_DHRS;
    asm("sti");
    //
    port->cmd_issue = 1 << 0;

    uint32_t timeout = 1000000;
    while (timeout--) {
        if (port->interrupt_status & 1) {
            break;
        }
        if (port->sata_error) {
            printk("SATA ERROR: %08x\n", port->sata_error);
            return;
        }
        asm("pause");
    }

    if (timeout == 0) {
        printk("SATA TIMEOUT\n");
        return;
    }

    if (sata->cmd_list0->prd_byte_count != 512) {
        printk("SATA PRD BYTE COUNT: %08x\n", sata->cmd_list0->prd_byte_count);
        return;
    }

    printk("identify data %08x\n", sata->data_vaddr);
    uint16_t* identify = (uint16_t*)sata->data_vaddr;

    // 第49个word的第8个bit位表示是否支持DMA
    // 第83个word的第10个bit位表示是否支持LBA48，为1表示支持。
    // 第100~103个word的八个字节表示user的LBA最大值
    if ((identify[49] & (1 << 8)) != 0) {
        sata->dma = 1;
    } else {
        panic("your sata disk drive do not support DMA");
    }

    uint64_t max_lba = 0;

    if ((identify[83] & (1 << 10)) != 0) {
        sata->lba48 = 1;
        max_lba = *(uint64_t*)(identify + 100);
    } else {
        panic("your sata disk drive do not support LBA48");
        // max_lba = (identify[61] << 16) | identify[60];
    }

    sata->max_lba = max_lba;

    printk("%s %s size: %u MB ", sata->dma == 1 ? "DMA" : "", sata->lba48 == 1 ? "LBA48" : "LBA28",
           (max_lba * 512) >> 20);

    uint16_t sata_major = identify[80];
    uint16_t sata_minor = identify[81];
    if (sata_major & (1 << 8)) {
        printk("ATA8-ACS ");
    }
    if (sata_major & (1 << 7)) {
        printk("ATA/ATAPI-7 ");
    }
    if (sata_major & (1 << 6)) {
        printk("ATA/ATAPI-6 ");
    }
    if (sata_major & (1 << 5)) {
        printk("ATA/ATAPI-5 ");
    }
    if (sata_major & (1 << 4)) {
        printk("ATA/ATAPI-4 ");
    }

    printk("[Major %04X Minor %02X]\n", sata_major, sata_minor);

    char s[64];
    sata_read_identify_string(identify, 10, 19, s);
    printk("SN: %s\n", s);

    sata_read_identify_string(identify, 23, 26, s);
    printk("Firmware Revision: %s\n", s);

    sata_read_identify_string(identify, 27, 46, s);
    printk("HD Model: %s\n", s);

    // while (!sata_irq_triggered) {
    //     // asm("sti");
    //     asm("pause");
    // }

    // 1. 等待端口空闲（确保上一条命令完全完成）
    while (port->cmd_issue & 1) {
        // 命令位仍被置位，等待清除
    }
    while (port->sata_active & 1) {
        // 等待active位清除
    }

    memset(fis, 0, sizeof(ahci_fis_reg_h2d_t));
    fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = SATA_CMD_IDENTIFY;
    fis->device = SATA_DEVICE_LBA;

    sata->cmd_list0->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    sata->cmd_list0->prdtl = 1;
    sata->cmd_list0->w = 0;  // read
    sata->cmd_list0->a = 0;  // ata device
    sata->cmd_list0->prd_byte_count = 0;
    // 清除中断状态
    // 写1清0
    port->interrupt_status = port->interrupt_status;

    port->interrupt_enable = AHCI_INTERRUPT_ENABLE_DHRS;

    //
    port->cmd_issue = 1 << 0;
}

sata_device_t sata_devices[MAX_SATA_DEVICES] = {0};

void sata_read_identify_string(const uint16_t* identify, int bgn, int end, char* buf) {
    const char* p = (const char*)(identify + bgn);
    int i = 0;
    for (; i <= (end - bgn); i++) {
        buf[2 * i + 1] = p[0];
        buf[2 * i + 0] = p[1];
        p += 2;
    }
    buf[i] = 0;
}
