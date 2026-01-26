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

int max_sata_devices = 0;
sata_device_t sata_devices[MAX_SATA_DEVICES] = {0};

uint8_t sector[512];
void init_sata_device(ahci_hba_t* hba, ahci_port_t* port, int port_index) {
    int index = max_sata_devices;

    assert(hba != NULL);
    assert(port != NULL);
    assert(port_index >= 0 && port_index < AHCI_PORT_COUNT);
    assert(index >= 0 && index < MAX_SATA_DEVICES);

    sata_device_t* sata = sata_devices + index;
    memset(sata, 0, sizeof(sata_device_t));
    sata->hba = hba;
    sata->port = port;
    sata->index = index;
    sata->port_index = port_index;

    init_completion(&sata->completion);

    printk("ahci port clb %08x fb %08x sata_status 0x%08X signature %08X\n", port->cmd_list_base, port->fis_base,
           port->sata_status, port->signature);

    vaddr_t cmd_list_base = (vaddr_t)ioremap(PAGE_ALIGN(port->cmd_list_base), 0x1000);
    vaddr_t fis_base = 0;

    // fis_base大概率和cmd_list_base在同一个页上
    // 如果不在同一个页上，那么需要单独分配空间
    if (PAGE_ALIGN(port->cmd_list_base) == PAGE_ALIGN(port->fis_base)) {
        fis_base = cmd_list_base;
    } else {
        fis_base = (vaddr_t)ioremap(PAGE_ALIGN(port->fis_base), 0x1000);
    }

    // 算出各自在页内的偏移
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

    sata->cmd_table_paddr = (paddr_t)page2pa(alloc_one_page(0));
    sata->cmd_table_vaddr = (ahci_cmd_table_t*)ioremap(sata->cmd_table_paddr, PAGE_SIZE);
    memset((void*)sata->cmd_table_vaddr, 0, PAGE_SIZE);

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

    //
    sata_identify(sata);

    if (!sata->lba48) {
        return;
    }

    if (!sata->dma) {
        return;
    }

    max_sata_devices += 1;

    // memset(sector, 0xCC, 512);
    // sata_dma_read(sata, 0, 1, (paddr_t)va2pa(sector));
    // printk("sector %08x\n", (uint32_t*)sector);
}

uint64_t sata_irq_cnt = 0;
void sata_irq_handler(unsigned int irq, pt_regs_t* regs, void* dev_id) {
    for (int i = 0; i < max_sata_devices; i++) {
        sata_device_t* sata = sata_devices + i;
        ahci_port_t* port = sata->port;
        assert(port != NULL);

        sata_irq_cnt += 1;

        //
        uint32_t interrupt_status = port->interrupt_status;
        if (0 == interrupt_status) {
            continue;
        }

        printk("SATA[%u] IRQ[%u] is %08x cnt %lu\n", i, irq, interrupt_status, sata_irq_cnt);
        if (interrupt_status & AHCI_INTERRUPT_STATUS_DHRS) {
            //
        }

        complete(&sata->completion);

        port->interrupt_status = interrupt_status;
    }

    ioapic_eoi();
}

bool sata_ready(sata_device_t* sata) {
    if (sata->port->task_file_data & 0x80) {
        return false;
    }
    return true;
}
int sata_wait_ready(sata_device_t* sata) {
    assert(sata != NULL);
    ahci_port_t* port = sata->port;
    assert(port != NULL);

    // 清除error 和 中断状态
    port->sata_error = port->sata_error;
    port->interrupt_status = port->interrupt_status;

    uint32_t timeout = 1000000;
    while (timeout--) {
        if (sata_ready(sata)) {
            return 0;
        }
        asm("pause");
    }
    return -1;
}

int sata_dma_read(sata_device_t* sata, uint64_t lba, uint32_t sectors, vaddr_t vaddr) {
    assert(sata != NULL);
    assert(sectors <= 4);
    assert(lba + sectors <= sata->max_lba);

    ahci_port_t* port = sata->port;
    assert(port != NULL);

    uint32_t bytes = sectors * 512;

    //
    if (0 != sata_wait_ready(sata)) {
        panic("sata wait ready timeout");
    }

    //
    sata->prdte0->data_base = va2pa(vaddr);
    sata->prdte0->data_byte_count = (bytes - 1) | 1;
    sata->prdte0->ioc = 1;

    //
    ahci_fis_reg_h2d_t* fis = (ahci_fis_reg_h2d_t*)sata->cmd_table0->cmd_fis;
    memset(fis, 0, sizeof(ahci_fis_reg_h2d_t));
    fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = SATA_CMD_READ_DMA_EXT;
    fis->device = SATA_DEVICE_LBA;
    fis->lba0 = (lba >> (0 * 8)) & 0xFF;
    fis->lba1 = (lba >> (1 * 8)) & 0xFF;
    fis->lba2 = (lba >> (2 * 8)) & 0xFF;
    fis->lba3 = (lba >> (3 * 8)) & 0xFF;
    fis->lba4 = (lba >> (4 * 8)) & 0xFF;
    fis->lba5 = (lba >> (5 * 8)) & 0xFF;

    //
    fis->count_low = (sectors >> 0) & 0xFF;
    fis->count_high = (sectors >> 8) & 0xFF;

    //
    sata->cmd_list0->cfl = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    sata->cmd_list0->prdtl = 1;
    sata->cmd_list0->w = 0;  // read
    sata->cmd_list0->a = 0;  // ata device
    sata->cmd_list0->c = 1;
    sata->cmd_list0->prd_byte_count = 0;

    //
    port->sata_error = port->sata_error;
    port->interrupt_status = port->interrupt_status;

    //
    port->interrupt_enable = AHCI_INTERRUPT_ENABLE_DHRS;

    //
    port->cmd_issue = 1 << 0;

    return 0;
}

void sata_identify(sata_device_t* sata) {
    assert(sata != NULL);

    paddr_t data_paddr = (paddr_t)page2pa(alloc_one_page(0));
    vaddr_t data_vaddr = (vaddr_t)ioremap(data_paddr, PAGE_SIZE);
    memset((void*)data_vaddr, 0, PAGE_SIZE);

    sata->prdte0->data_base = data_paddr;
    sata->prdte0->data_byte_count = (512 - 1) | 1;
    sata->prdte0->ioc = 0;

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
    sata->cmd_list0->c = 1;  // 自动清除BSY位
    sata->cmd_list0->prd_byte_count = 0;

    // 清除中断状态
    // 写1清0
    ahci_port_t* port = sata->port;
    assert(port != NULL);
    port->interrupt_status = port->interrupt_status;

#if 0
    //
    port->interrupt_enable = AHCI_INTERRUPT_ENABLE_DHRS;
#endif

    //
    port->cmd_issue = 1 << 0;

    uint32_t timeout = 1000000;
    while (timeout--) {
        if (((port->cmd_issue) & (1 << 0)) == 0) {
            break;
        }
        // 不启用中断也仍然会设置
        if (port->interrupt_status & 1) {
            printk("SATA INTERRUPT STATUS: %08x\n", port->interrupt_status);
            // 清除本次请求产生的中断
            port->interrupt_status = port->interrupt_status;
            break;
        }
        if (port->sata_error) {
            printk("SATA ERROR: %08x\n", port->sata_error);
            goto end;
        }
        asm("pause");
    }

    if (timeout == 0) {
        printk("SATA TIMEOUT\n");
        goto end;
    }

    if (sata->cmd_list0->prd_byte_count != 512) {
        printk("SATA PRD BYTE COUNT: %08x\n", sata->cmd_list0->prd_byte_count);
        goto end;
    }

    printk("identify data %08x\n", data_vaddr);
    uint16_t* identify = (uint16_t*)data_vaddr;

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

end:
    iounmap(data_vaddr);
    free_pages((unsigned long)pa2va(data_paddr));
}

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
