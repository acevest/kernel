/*
 * ------------------------------------------------------------------------
 *   File Name: ahci.c
 *      Author: Zhao Yanbai
 *              2026-01-16 21:32:08 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <pci.h>
#include <system.h>
#include <ioremap.h>
#include <string.h>

#include <sata.h>

#include <pci.h>
#include <list.h>

void init_ahci_device(pci_device_t* pci, int index) {
    assert(pci != NULL);
    assert(index >= 0);

    printk("found ahci[%d] sata pci %03d:%02d.%d #%02d %04X:%04X\n", index, pci->bus, pci->dev, pci->devfn,
           pci->intr_line, pci->vendor, pci->device);

    uint32_t bar5 = pci->bars[5];
    printk("ahci pci BAR5 value 0x%08X\n", bar5);
    for (int i = 0; i < 6; i++) {
        printk("  ahci pci BAR%u value 0x%08X\n", i, pci->bars[i]);
    }

    pci_write_config_word(0xFFFFFFFF, PCI_BAR5);
    uint32_t size = pci_read_config_word(PCI_BAR5);
    assert(size > 0);
#if 1
    size += 1;
#else
    size &= 0xFFFFFFF0;
    size = (~size) + 1;
#endif
    printk("ahci pci BAR5 size 0x%08X\n", size);

    uint32_t* mapped = ioremap(bar5, size);
    if (mapped == NULL) {
        panic("failed to map sata bar5\n");
        return;
    }
    printk("ahci pci BAR5 mapped to 0x%08X\n", mapped);

    struct {
        uint32_t offset;
        const char* name;
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
        uint32_t value = mapped[regs[i].offset / 4];
        printk("  ahci registers 0x%08X[%s]\n", value, regs[i].name);
    }

    ahci_hba_t* hba = (ahci_hba_t*)mapped;

    hba->global_hba_control |= AHCI_ENABLE;

    uint32_t cap = hba->capability;

    int num_ports = (cap & 0x1F) + 1;
    int num_cs = ((cap >> 8) & 0x1F) + 1;  // command slots
    int iss = (cap >> 20) & 0x0F;          // interface speed support
    int s64a = (cap >> 31) & 0x01;         // supports 64bit addressing
    printk("ahci ports %d cmd slots %d\n", num_ports, num_cs);
    printk("support 64bit addressing %s\n", s64a ? "Y" : "N");

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
        printk(" unknown interface speed support %02X\n", iss);
        break;
    }

    uint32_t ahci_pi = hba->ports_implemented;
    printk("ahci port implemented %08X\n", ahci_pi);

    uint32_t ahci_version = hba->version;
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
    printk("ahci version %s[%08x]\n", version, ahci_version);

    assert(sizeof(ahci_port_t) == 0x80);
    for (int i = 0; i < num_ports; i++) {
        if (ahci_pi & (1 << i)) {
            ahci_port_t* port = hba->ports + i;
            uint32_t sata_status = port->sata_status;
            uint32_t sata_det = (sata_status >> 0) & 0x0F;  // device detection
            uint32_t sata_spd = (sata_status >> 4) & 0x0F;  // current interface speed
            uint32_t sata_ipm = (sata_status >> 8) & 0x0F;  // interface power management

            if (sata_det != 3) {
                continue;
            }

            if (sata_ipm != 0x01) {
                continue;
            }

            switch (port->signature) {
            case SATA_SIGNATURE_ATA:
                extern void init_sata_device(ahci_hba_t * hba, ahci_port_t * port, int index);
                init_sata_device(hba, port, i);
                printk("SATA device detected at port %d\n", i);

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
                printk("unknown ahci device detected\n");
                break;
            }
        }
    }

    // TODO
    // 开启ahci的global_hba_control的interrupt enable位
    hba->global_hba_control |= AHCI_INTERRUPT_ENABLE;
}

void init_ahci() {
    // progif
    // 0x01 AHCI
    pci_init_device(0x0106, 0x01, init_ahci_device);
}
