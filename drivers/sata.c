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

    printk("found sata pci progif %02x %03d:%02d.%d #%02d %04X:%04X\n", pci->progif, pci->bus, pci->dev, pci->devfn,
           pci->intr_line, pci->vendor, pci->device);
    for (int i = 0; i < 6; i++) {
        printk("  sata pci BAR%u value 0x%08X\n", i, pci->bars[i]);
        // if (pci->bars[i] != 0) {
        //     assert((pci->bars[i] & 0x1) == 0x1);
        // }
    }
}
