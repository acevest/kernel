/*
 * ------------------------------------------------------------------------
 *   File Name: sata.c
 *      Author: Zhao Yanbai
 *              2025-12-31 20:29:10 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <pci.h>
#include <system.h>
#include <ioremap.h>
#include <string.h>

void init_sata() {
    pci_device_t* pci = pci_find_device_by_classcode(0x0106);
    if (pci == NULL) {
        printk("can not find pci classcode: %08x", 0x0106);
        printk("can not find sata controller");
        return;
    }

    // progif
    // 0x01 AHCI
    //
    // 另外读取 BAR5 的 CAP 寄存器，如果 CAP 寄存器的 31 位为 1，则表示支持 AHCI 模式。

    if (pci->progif == 0x01) {
        printk("AHCI mode supported\n");
    } else {
        printk("AHCI mode not supported\n");
    }

    printk("found ahci sata pci progif %02x %03d:%02d.%d #%02d %04X:%04X\n", pci->progif, pci->bus, pci->dev,
           pci->devfn, pci->intr_line, pci->vendor, pci->device);

    uint32_t bar5 = pci->bars[5];
    printk("ahci pci BAR5 value 0x%08X\n", bar5);
    for (int i = 0; i < 6; i++) {
        printk("  ahci pci BAR%u value 0x%08X\n", i, pci->bars[i]);
        // if (pci->bars[i] != 0) {
        //     assert((pci->bars[i] & 0x1) == 0x1);
        // }
    }

    pci_write_config_word(0xFFFFFFFF, PCI_BAR5);
    uint32_t size = pci_read_config_word(PCI_BAR5);
    printk("ahci pci BAR5 size 0x%08X\n", size);

    assert(size > 0);
    size += 1;

    uint32_t* mapped = ioremap(bar5, size);
    if (mapped == NULL) {
        panic("failed to map sata bar5\n");
        return;
    }
    printk("ahci pci BAR5 mapped to 0x%08X\n", mapped);

    struct {
        uint32_t offset;
        const char* name;
        uint32_t value;
    } regs[] = {
        {0x00, "CAP"},        //
        {0x04, "GHC"},        //
        {0x08, "IS"},         //
        {0x0C, "PI"},         //
        {0x10, "VS"},         //
        {0x14, "CCC_CTL"},    //
        {0x18, "CCC_PORTS"},  //
        {0x1C, "EM_LOC"},     //
        {0x20, "EM_CTL"},     //
        {0x24, "CAP2"},       //
        {0x28, "BOHC"},       //
    };

    for (int i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
        regs[i].value = mapped[regs[i].offset / 4];
        printk("ahci registers 0x%08X[%s]\n", regs[i].value, regs[i].name);
    }

    uint32_t cap = mapped[0x00 / 4];
    int num_ports = (cap & 0x1F) + 1;
    int num_cs = (cap >> 8) & 0x1F;  // command slots
    int iss = (cap >> 20) & 0x0F;    // interface speed support
    int s64a = (cap >> 31) & 0x01;
    printk("ahci num_ports %d num_cs %d\n", num_ports, num_cs);

    switch (iss) {
    case 0b0001:
        printk(" Gen1 (1.5 Gbps)\n");
        break;
    case 0b0010:
        printk(" Gen2 (3.0 Gbps)\n");
        break;
    case 0b0100:
        printk(" Gen3 (6.0 Gbps)\n");
        break;
    default:
        printk(" unknown iss %02X\n", iss);
        break;
    }
    printk("support 64bit addressing %s\n", s64a ? "Y" : "N");

    uint32_t ahci_pi = mapped[0x0C / 4];
    printk("ahci port implemented %08X\n", ahci_pi);

    uint32_t ahci_version = mapped[0x10 / 4];
    printk("ahci version 0x%08X\n", ahci_version);

    char version[16];
    switch (ahci_version) {
    case 0x00000905:
        strcpy(version, "0.95");
        break;
    case 0x00010000:
        strcpy(version, "1.0");
        break;
    case 0x00010100:
        strcpy(version, "1.1");
        break;
    case 0x00010200:
        strcpy(version, "1.2");
        break;
    case 0x00010300:
        strcpy(version, "1.3");
        break;
    case 0x00010301:
        strcpy(version, "1.3.1");
        break;
    default:
        strcpy(version, "unknown");
        break;
    }
    printk("ahci version %s\n", version);

    typedef struct {
        uint32_t cmd_list_base;
        uint32_t cmd_list_base_upper;
        uint32_t fis_base;
        uint32_t fis_base_upper;
        uint32_t interrupt_status;
        uint32_t interrupt_enable;
        uint32_t command_and_status;
        uint32_t reserved;
        uint32_t task_file_data;
        uint32_t signature;
        uint32_t sata_status;
        uint32_t sata_control;
        uint32_t sata_error;
        uint32_t sata_active;
        uint32_t cmd_issue;
        uint32_t sata_notification;
        uint32_t fis_base_switch_control;
        uint32_t device_sleep;
        uint32_t reserved2[10];
        uint32_t vendor_specific[4];
    } ahci_port_t;

    assert(sizeof(ahci_port_t) == 0x80);
    for (int i = 0; i < 32; i++) {
        if (ahci_pi & (1 << i)) {
            // printk("ahci port %d implemented\n", i);
            ahci_port_t* port = (ahci_port_t*)(mapped + (0x100 + i * 0x80) / 4);
            // printk("ahci port %d dt 0x%08X\n", i, port->command_and_status);
            uint32_t sata_status = port->sata_status;
            uint32_t sata_det = (sata_status >> 0) & 0x0F;
            uint32_t sata_ipm = (sata_status >> 8) & 0x0F;

            if (sata_det != 3) {
                continue;
            }

            if (sata_ipm != 0x01) {
                continue;
            }

            printk("ahci port %d clb %08x fb %08x sata_status 0x%08X signature %08X\n", i, port->cmd_list_base,
                   port->fis_base, sata_status, port->signature);

#define SATA_SIGNATURE_ATA 0x00000101
#define SATA_SIGNATURE_ATAPI 0xEB140101
#define SATA_SIGNATURE_SEMB 0xC33C0101
#define SATA_SIGNATURE_PM 0x96690101

            switch (port->signature) {
            case SATA_SIGNATURE_ATA:
                printk("SATA device detected\n");
                break;
            case SATA_SIGNATURE_ATAPI:
                printk("SATAPI device detected\n");
                break;
            case SATA_SIGNATURE_SEMB:
                printk("SEMB device detected\n");
                break;
            case SATA_SIGNATURE_PM:
                printk("PM device detected\n");
                break;
            default:
                printk("unknown device detected\n");
                break;
            }
        }
    }
}
