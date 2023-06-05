/*
 * ------------------------------------------------------------------------
 *   File Name: ide.c
 *      Author: Zhao Yanbai
 *              Sat May 24 16:30:38 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <ide.h>
#include <irq.h>
#include <semaphore.h>
#include <task.h>
#include <wait.h>

ide_pci_controller_t ide_pci_controller[NR_IDE_CONTROLLER];

unsigned int IDE_CHL0_CMD_BASE = 0x1F0;
unsigned int IDE_CHL1_CMD_BASE = 0x170;

unsigned int IDE_CHL0_CTL_BASE = 0x3F6;
unsigned int IDE_CHL1_CTL_BASE = 0x376;

// 《PCI IDE Controller Specification》
// 《Programming Interface for Bus Master IDE Controller》
void ide_pci_init(pci_device_t *pci) {
    unsigned int v;

    v = pci_read_config_word(pci_cmd(pci, PCI_COMMAND));
    // printk(" ide pci command %04x\n", v);

    v = pci_read_config_byte(pci_cmd(pci, PCI_PROGIF));
    printd("ide pci program interface %02x\n", v);

    unsigned int iobase = pci_read_config_long(pci_cmd(pci, PCI_BAR4));

    for (int i = 0; i < NR_IDE_CONTROLLER; i++) {
        iobase += i * 8;  // secondary channel 需要加8
        printd("ide pci Base IO Address Register %08x\n", iobase);
        iobase &= 0xFFFC;  // 最低为0是内存地址为1是端口地址
        ide_pci_controller[i].bus_iobase = iobase;
        ide_pci_controller[i].bus_cmd = iobase + PCI_IDE_CMD;
        ide_pci_controller[i].bus_status = iobase + PCI_IDE_STATUS;
        ide_pci_controller[i].bus_prdt = iobase + PCI_IDE_PRDT;
        ide_pci_controller[i].prdt = (prdte_t *)alloc_one_page(0);

        printd("BARS: ");
        for (int j = 0; j < BARS_CNT; ++j) {
            printd("%08x ", pci->bars[j]);
            pci->bars[j] &= (~1UL);
        }
        printd("\n");

        ide_pci_controller[i].pci = pci;
    }

    IDE_CHL0_CMD_BASE = pci->bars[0] ? pci->bars[0] : IDE_CHL0_CMD_BASE;
    IDE_CHL0_CTL_BASE = pci->bars[1] ? pci->bars[1] : IDE_CHL0_CTL_BASE;

    IDE_CHL1_CMD_BASE = pci->bars[2] ? pci->bars[2] : IDE_CHL1_CMD_BASE;
    IDE_CHL1_CTL_BASE = pci->bars[3] ? pci->bars[3] : IDE_CHL1_CTL_BASE;

    printd("ide channel 0: cmd %04x ctl %04x\n", IDE_CHL0_CMD_BASE, IDE_CHL0_CTL_BASE);
    printd("ide channel 1: cmd %04x ctl %04x\n", IDE_CHL1_CMD_BASE, IDE_CHL1_CTL_BASE);
}

// void ide_status() {
//     u8_t idest = inb(REG_STATUS(0));
//     u8_t pcist = inb(drv.bus_status);
//     printk(" ide status %02x pci status %02x\n", idest, pcist);
// }

// void ide_debug() {
//     unsigned int nsect = 1;
//     char *buf = kmalloc(1 * SECT_SIZE, 0);
//     if (buf == 0) panic("out of memory");

//     ide_do_read(0, nsect, buf);

//     u16_t sig = *((u16_t *)(buf + 510));
//     printk("%s SIG: %04x\n", __func__, sig);

//     kfree(buf);
// }
const char *pci_get_info(unsigned int classcode, unsigned int progif);
void init_pci_controller(unsigned int classcode) {
    pci_device_t *pci = pci_find_device_by_classcode(classcode);
    if (pci == NULL) {
        printk("can not find pci classcode: %08x", classcode);
        panic("can not find ide controller");
    }
    assert(pci->intr_line < 16);
    if (pci != 0 && pci->intr_line < 16) {
        printk("found pci %03d:%02d.%d #%02d %04X:%04X ProgIF %02x %s\n", pci->bus, pci->dev, pci->devfn,
               pci->intr_line, pci->vendor, pci->device, pci->progif, pci_get_info(pci->classcode, pci->progif));
        // printk("found pci vendor %04x device %04x class %04x intr %d progif: %x\n", pci->vendor, pci->device,
        //        pci->classcode, pci->intr_line, pci->progif);
        // printl(17, "found pci vendor %04x device %04x class %04x intr %d", pci->vendor, pci->device,
        // pci->classcode,
        // pci->intr_line);
        ide_pci_init(pci);
        // while (1) asm("cli;hlt;");
    }
}

extern semaphore_t disk_intr_sem;

extern void *mbr_buf;
extern uint32_t disk_request_cnt;
extern uint32_t disk_handled_cnt;

uint8_t ata_pci_bus_status();

volatile uint32_t disk_inter_cnt = 0;

void ata_dma_stop(int channel);
void ide_irq_bh_handler(void *arg) {
    disk_inter_cnt++;

    int channel = (int)arg;

    // printl(MPL_IDE, "disk req %u consumed %u irq %u", disk_request_cnt, disk_handled_cnt, disk_inter_cnt);
    printlxy(MPL_IDE, MPO_IDE, "disk irq %u req %u consumed %u ", disk_inter_cnt, disk_request_cnt, disk_handled_cnt);

    // up里不会立即重新调度进程
    up(&disk_intr_sem);
}

void ide_irq_handler(unsigned int irq, pt_regs_t *regs, void *devid) {
    // printk("ide irq %d handler pci status: 0x%02x\n", irq, ata_pci_bus_status());

    int channel = irq == 14 ? 0 : 1;
    ata_dma_stop(channel);

    add_irq_bh_handler(ide_irq_bh_handler, &channel);
}

void ide1_irq_handler(unsigned int irq, pt_regs_t *regs, void *devid) { panic("ide 0"); }

void ide_ata_init();
void ide_init() {
    memset(ide_pci_controller, 0, sizeof(ide_pci_controller[0]) * NR_IDE_CONTROLLER);

    // 读PCI里 IDE相关寄存器的配置
    // init_pci_controller(0x0106);
    init_pci_controller(0x0101);
    // init_pci_controller(0x7010);

    // 读IDE 硬盘的identity
    ide_ata_init();

    request_irq(0x0E, ide_irq_handler, "hard", "IDE");

    request_irq(0x0F, ide_irq_handler, "hard", "IDE");
}
