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

/*
高技术配置（英语：Advanced Technology Attachment，简称“ATA”）与由集成驱动电子设备（英语：Integrated Drive
Electronics，简称IDE）技术实现的磁盘驱动器关系最密切。
ATA里的AT是源自PC/AT
*/

/*
IDE和ATA实际上是同一种技术，只是在名称上有所不同。IDE代表“集成驱动器电子设备”（Integrated Drive Electronics），
而ATA代表“高级技术附件”（Advanced Technology Attachment）。这两个名称都是指代用于连接和控制计算机硬盘驱动器的接口标准。
这种技术最初在1986年由Western Digital公司为IBM的PC/AT设计，当时的目标是通过将控制器直接集成到硬盘驱动器上，
来在性能和成本上优于当时的SCSI接口标准。这种设计被命名为IDE。 后来，为了使这种技术的标准化，American National Standards
Institute（ANSI）制定了一系列的ATA标准。因此，从技术角度来说，IDE和ATA实际上是同一种事物，只是在历史发展过程中，IDE的名称更早被提出，而ATA则是后来为了标准化而提出的名称。
至于为什么会出现混用的情况，这主要是因为在市场上，很多人已经习惯了使用IDE这个名称，而ATA则是在技术文档和标准中更常用。实际上，如果你去购买硬盘驱动器，你可能会看到IDE/ATA这样的标签，这只是为了向用户表明，这种驱动器符合ATA的标准，同时也是IDE的一种。
总的来说，IDE和ATA就像两面硬币，代表的是同一种技术，只是在名称上有所不同，因此会出现混用的情况。
*/

/*
除了硬盘之外，IDE接口还可以用于连接光盘驱动器（如CD-ROM、DVD-ROM等）、ZIP驱动器等存储设备。这些设备都使用了类似于ATA的传输协议和控制方式。为了适应光盘驱动器等设备的特性，ATA标准得到了扩展，形成了ATAPI（ATA
Packet Interface）标准。
ATAPI可以看作是ATA标准的一个补充，它允许在ATA基础上增加了一些特定的命令集，用于支持光盘驱动器等设备的特殊功能，如媒体播放控制等。
因此，当我们说IDE可以接光盘驱动器时，实际上是指IDE接口也支持兼容ATAPI标准的设备。换句话说，ATAPI是ATA标准的一个扩展，IDE接口既可以用于连接硬盘（使用ATA标准），也可以用于连接光盘驱动器（使用ATAPI标准）。
*/

/*
在IDE的发展过程中，早期的IDE接口确实未必支持光盘驱动器。这主要是因为当时的IDE接口和ATA标准主要针对的是硬盘驱动器，尚未考虑涉及光盘驱动器等其他存储设备的需求。不过，随着技术的发展和市场需求的变化，IDE接口逐渐支持了ATAPI标准，从而可以兼容光盘驱动器等设备。
总的来说，现在市场上的IDE接口几乎都支持光盘驱动器，因为ATAPI已经成为了IDE接口的一个通用标准。但在IDE技术刚推出时，某些早期版本的IDE接口确实可能不支持光盘驱动器。
*/

/*
ATA（Advanced Technology Attachment）的发展历程主要经历了以下阶段：

ATA-1：在1986年，Western
Digital为IBM的PC/AT设计了IDE接口，这就是最早的ATA标准，被称为ATA-1。这个标准的传输速度最高为8.3MB/s。

ATA-2：1994年，ATA-1标准被改进为ATA-2，也被称为Fast ATA或Enhanced IDE
(EIDE)。这个标准提供了更高的数据传输速度（最高16.6MB/s），并增加了支持ATAPI设备（如CD-ROM驱动器）的能力。

ATA-3：ATA-2后不久，ATA-3标准被发布，主要增加了一些安全特性，如设备密码保护等。

Ultra ATA/ATA-4：1998年，ATA标准再次被改进，发布了Ultra ATA/ATA-4，传输速度最高达到33MB/s。

Ultra ATA/66（ATA-5）、Ultra ATA/100（ATA-6）和Ultra
ATA/133（ATA-7）：随后的几年里，ATA标准不断改进，发布了ATA-5、ATA-6和ATA-7，传输速度分别提升到66MB/s、100MB/s和133MB/s。

Serial ATA (SATA)：2003年，为了解决并行ATA接口在速度和设计上的限制，Serial
ATA（SATA）标准被发布，这是一种全新的基于串行技术的接口。SATA提供了更高的传输速度（最初为150MB/s，后续版本可达600MB/s），并且接口更小，设计更灵活。

总的来说，ATA的发展历程是一个不断追求更高传输速度，更高兼容性，以及更易于设计和使用的过程。在这个过程中，ATA标准从最初的IDE发展到了今天的SATA，为现代计算机的发展打下了重要基础。
*/

/*
最完善的IDE接口（并行ATA接口）支持到ATA-7标准。ATA-7标准也被称为Ultra
ATA/133，它提供了最高133MB/s的数据传输速度。在此之后，IDE接口逐渐被新的串行ATA接口（SATA）所取代，SATA成为了现代计算机硬盘和其他存储设备的主要接口标准。虽然SATA仍然遵循ATA的基本架构和命令集，但它采用了不同的物理接口和传输技术，因此不能直接与IDE接口兼容。
 */

/*
在IDE出现之前，计算机主要依赖ST-506/412接口和SCSI接口来连接硬盘和其他存储设备。这些接口在当时存在以下问题：

1. 性能：ST-506/412接口的传输速度相对较慢（最高约为5MB/s），不能满足日益增长的性能需求。

2. 成本：尽管SCSI接口在性能上优于ST-506/412接口，但其成本较高，主要应用于高性能工作站和服务器领域。对于普通消费者而言，
SCSI接口的成本可能难以承受。

3. 复杂性：早期的硬盘接口需要单独的控制器卡，用户需要购买和安装额外的硬件。此外，对于SCSI接口，
设备的配置和管理相对复杂。 IDE接口的出现带来了以下优点，解决了上述核心问题：

1. 性能提升：通过将硬盘控制器集成到硬盘驱动器本身，IDE接口在性能上相较于ST-506/412接口有了很大提升。
随着技术的发展，后续的ATA标准（如ATA-2、ATA-3等）进一步提高了数据传输速度。

2. 成本降低：由于将控制器集成到硬盘驱动器上，IDE接口减少了额外硬件的需求，从而降低了成本。
这使得IDE接口成为了普通消费者更容易接受的选择。

3. 易用性提高：IDE接口简化了设备的配置和管理，无需单独的控制器卡，
用户只需将硬盘驱动器直接连接到计算机主板上的IDE插槽即可。

4. 兼容性：随着ATAPI标准的引入，IDE接口不仅可以连接硬盘驱动器，还可以连接光盘驱动器、 ZIP驱动器等其他存储设备，
提高了其兼容性和应用范围。

总之，IDE接口的出现解决了早期硬盘接口在性能、成本和易用性方面的核心问题，为个人计算机市场的发展奠定了基础。
*/

/*
IDE是一种计算机系统接口，主要用于硬盘和CD-ROM，本意为“把控制器与盘体集成在一起的硬盘”。数年以前PC主机使用的硬盘，大多数都是IDE兼容的，只需用一根电缆将它们与主板或适配器连起来就可以了，而目前主要接口为SATA接口。而在SATA技术日益发展下，没有ATA的主板已经出现，而且Intel在新型的芯片组中已经不默认支持ATA接口，主机版厂商需要另加芯片去对ATA作出支持（通常是为了兼容旧有硬盘和光盘驱动器）。
SATA（Serial ATA）于2002年推出后，原有的ATA改名为PATA（并行高技术配置，Parallel ATA）。2013年12月29日，
西部数据正式停止PATA硬盘供应，而希捷科技则已停售产多年，这意味着1986年设计的PATA接口在经历27年后正式退出历史舞台。
一般说来，ATA是一个控制器技术，而IDE是一个匹配它的磁盘驱动器技术，但是两个术语经常可以互用。ATA是一个花费低而性能适中的接口，主要是针对台式机而设计的，销售的大多数ATA控制器和IDE磁盘都是更高版本的，称为ATA-2和ATA-3，与之匹配的磁盘驱动器称为增强的IDE。
把盘体与控制器集成在一起的做法，减少了硬盘接口的电缆数目与长度，数据传输的可靠性得到了增强，硬盘制造起来变得更容易，因为厂商不需要再担心自己的硬盘是否与其他厂商生产的控制器兼容，对用户而言，硬盘安装起来也更为方便。
*/

// 随着SATA技术的出现，之前所有的ATA就被称为PATA
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
        if (pci->bars[i] != 0) {
            assert((pci->bars[i] & 0x1) == 0x1);
        }
    }

    // BAR4开始有16个端口，有2组，每组8个，分别控制Primary和Secondary通道的DMA
    // BAR5: SATA AHCI Base Address
    // 虽然iobase是uint32_t，但在PC机上最多有65536个端口，也就是有效位数为16bit
    // 所以这里用0xFFFE或0xFFFFFFFE其实都可以
    uint32_t iobase = pci->bars[4] & 0xFFFFFFFE;  // 最低为0是内存地址为1是端口地址

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
        ide_pci_controller[i].bus_iobase = iobase;
        ide_pci_controller[i].bus_cmd = iobase + PCI_IDE_CMD;
        ide_pci_controller[i].bus_status = iobase + PCI_IDE_STATUS;
        ide_pci_controller[i].bus_prdt = iobase + PCI_IDE_PRDT;
        ide_pci_controller[i].prdt = (prdte_t *)page2va(alloc_one_page(0));

        ide_pci_controller[i].pci = pci;
    }

    IDE_CHL0_CMD_BASE = pci->bars[0] ? (pci->bars[0] & 0xFFFFFFFE) : IDE_CHL0_CMD_BASE;
    IDE_CHL0_CTL_BASE = pci->bars[1] ? (pci->bars[1] & 0xFFFFFFFE) : IDE_CHL0_CTL_BASE;

    IDE_CHL1_CMD_BASE = pci->bars[2] ? (pci->bars[2] & 0xFFFFFFFE) : IDE_CHL1_CMD_BASE;
    IDE_CHL1_CTL_BASE = pci->bars[3] ? (pci->bars[3] & 0xFFFFFFFE) : IDE_CHL1_CTL_BASE;

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
    init_pci_controller(0x0101);

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
