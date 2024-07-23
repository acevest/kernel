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
#if 0
    uint32_t v;
    uint32_t cmd;

    cmd = pci_cmd(pci, PCI_COMMAND);
    v = pci_read_config_word(cmd);
    // printk(" ide pci command %04x\n", v);

    cmd = pci_cmd(pci, PCI_PROGIF);
    v = pci_read_config_byte(cmd);

    printd("ide pci program interface %02x %02X\n", v);
#endif
    printd("PCI %03d:%02d.%d %02X #%02d %04X:%04X\n", pci->bus, pci->dev, pci->devfn, pci->progif, pci->intr_line,
           pci->vendor, pci->classcode);

    for (int i = 0; i < 6; i++) {
        printd("ide pci BAR%u value 0x%08X\n", i, pci->bars[i]);
    }

    // BAR4开始有16个端口，有2组，每组8个，分别控制Primary和Secondary通道的DMA
    // BAR5: SATA AHCI Base Address
    uint32_t iobase = pci->bars[4];

    for (int i = 0; i < NR_IDE_CONTROLLER; i++) {
        INIT_MUTEX(&ide_pci_controller[i].request_mutex);
        ide_pci_controller[i].request_queue.count = 0;
        INIT_LIST_HEAD(&ide_pci_controller[i].request_queue.list);
        semaphore_init(&ide_pci_controller[i].request_queue.sem, 0);
        init_completion(&ide_pci_controller[i].intr_complete);
        ide_pci_controller[i].intr_complete.name = i == 0 ? "ide0_intr_complete" : "ide1_intr_complete";

        atomic_set(&ide_pci_controller[i].request_cnt, 0);
        atomic_set(&ide_pci_controller[i].irq_cnt, 0);
        atomic_set(&ide_pci_controller[i].consumed_cnt, 0);

        iobase += i * 8;  // secondary channel 需要加8
        printd("ide pci Base IO Address Register %08x\n", iobase);
        // 虽然iobase是uint32_t，但在PC机上最多有65536个端口，也就是有效位数为16bit
        // 所以这里用0xFFFC或0xFFFFFFFC其实都可以

        iobase &= 0xFFFC;  // 最低为0是内存地址为1是端口地址
        ide_pci_controller[i].bus_iobase = iobase;
        ide_pci_controller[i].bus_cmd = iobase + PCI_IDE_CMD;
        ide_pci_controller[i].bus_status = iobase + PCI_IDE_STATUS;
        ide_pci_controller[i].bus_prdt = iobase + PCI_IDE_PRDT;
        ide_pci_controller[i].prdt = (prdte_t *)alloc_one_page(0);

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
        printk("found pci %03d:%02d.%d #%02d %04X:%04X progif %02x %s\n", pci->bus, pci->dev, pci->devfn,
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

extern void *mbr_buf;

uint8_t ata_pci_bus_status();

void ata_dma_stop(int channel);
void ide_irq_bh_handler(void *arg) {
    int channel = (int)arg;

    // printk("channel %08x\n", channel);
    assert(channel <= 1);
    assert(channel >= 0);

    ide_pci_controller_t *ide_ctrl = ide_pci_controller + channel;
    // printlxy(MPL_IDE, MPO_IDE, "disk irq %u req %u consumed %u ", disk_inter_cnt, disk_request_cnt,
    // disk_handled_cnt);

    printlxy(MPL_IDE0 + channel, MPO_IDE, "IDE%d req %u irq %u consumed %u", channel,
             atomic_read(&(ide_ctrl->request_cnt)), ide_ctrl->irq_cnt, ide_ctrl->consumed_cnt);

    // 之前这里是用up()来唤醒磁盘任务
    // 但在中断的底半处理，不应该切换任务，因为会引起irq里的reenter问题，导致不能再进底半处理，也无法切换任务
    // 所以就移除了up()里的 schedule()
    // 后来就改用完成量来通知磁盘任务，就不存在这个问题了

    // complete会唤醒进程，但不会立即重新调度进程
    complete(&ide_ctrl->intr_complete);
}

void ide_irq_handler(unsigned int irq, pt_regs_t *regs, void *devid) {
    // printk("ide irq %d handler pci status: 0x%02x\n", irq, ata_pci_bus_status());

    int channel = irq == 14 ? 0 : 1;

    ide_pci_controller_t *ide_ctrl = ide_pci_controller + channel;
    atomic_inc(&ide_ctrl->irq_cnt);

    ata_dma_stop(channel);

    add_irq_bh_handler(ide_irq_bh_handler, (void *)channel);
}

void ide_ata_init();
void ide_init() {
    memset(ide_pci_controller, 0, sizeof(ide_pci_controller[0]) * NR_IDE_CONTROLLER);

    // 读PCI里 IDE相关寄存器的配置
    // init_pci_controller(0x0106);
    init_pci_controller(0x0101);
    // init_pci_controller(0x7010);

    request_irq(0x0E, ide_irq_handler, "hard", "IDE");

    request_irq(0x0F, ide_irq_handler, "hard", "IDE");

    // 读IDE 硬盘的identity
    ide_ata_init();
}

ide_drive_t *ide_get_drive(dev_t dev) {
    int major = DEV_MAJOR(dev);
    int minor = DEV_MINOR(dev);

    int drv_no = (minor & 0xFFFF) >> 8;

    assert(major == DEV_MAJOR_DISK);
    assert(minor >= 0);
    assert(drv_no < MAX_IDE_DRIVE_CNT);

    ide_drive_t *drv = ide_drives + drv_no;

    return drv;
}
