/*
 *--------------------------------------------------------------------------
 *   File Name: pci.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sun Mar  8 21:33:13 2009
 * Last Update: Sun Mar  8 21:33:13 2009
 *
 *--------------------------------------------------------------------------
 */
#include <io.h>
#include <pci.h>
#include <printk.h>
#include <system.h>
#include <types.h>

LIST_HEAD(pci_devs);

const char *pci_get_info(unsigned int classcode, unsigned int progif);

int pci_read_config_byte(int cmd) {
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inb(PCI_DATA + (PCI_GET_CMD_REG(cmd) & 3));
}

int pci_read_config_word(int cmd) {
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inw(PCI_DATA + (PCI_GET_CMD_REG(cmd) & 2));
}

int pci_read_config_long(int cmd) {
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inl(PCI_DATA);
}

void pci_write_config_byte(int value, int cmd) {
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    outb(value & 0xFF, PCI_DATA + (cmd & 3));
}

void pci_write_config_word(int value, int cmd) {
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    outw(value & 0xFFFF, PCI_DATA + (cmd & 2));
}

void pci_write_config_long(int value, int cmd) {
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    outl(value, PCI_DATA);
}

void scan_pci_bus(int bus) {
    u8 dev, devfn;
    u32 cmd;
    u32 v;
    int i;
    // printk("scanning pci bus %d\n", bus);

    for (dev = 0; dev < 32; dev++) {
        for (devfn = 0; devfn < 8; devfn++) {
            cmd = PCI_CMD(bus, dev, devfn, PCI_VENDORID);
            v = pci_read_config_word(cmd);
            if (v == 0xFFFF) {
                continue;
            }

            pci_device_t *pci = kmalloc(sizeof(pci_device_t), 0);
            if (0 == pci) {
                printk("no space to alloc for pci_device_t\n");
                continue;
            }

            pci->bus = bus;
            pci->dev = dev;
            pci->devfn = devfn;

            pci->vendor = v;

            cmd = PCI_CMD(bus, dev, devfn, PCI_DEVICEID);
            pci->device = pci_read_config_word(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_REVISION);
            pci->revision = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_PROGIF);
            pci->progif = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_CLASSCODE);
            pci->classcode = pci_read_config_word(cmd);

            cmd = PCI_CMD(bus, dev, devfn, 4);
            pci->command = pci_read_config_word(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_INTRLINE);
            pci->intr_line = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_INTRPIN);
            pci->intr_pin = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_HDRTYPE);
            pci->hdr_type = pci_read_config_byte(cmd);
            pci->hdr_type &= PCI_HDRTYPE_MASK;

            for (i = 0; i < BARS_CNT; ++i) {
                cmd = PCI_CMD(bus, dev, devfn, PCI_BAR0 + i * 4);
                pci->bars[i] = pci_read_config_long(cmd);
            }

            list_add_tail(&pci->list, &pci_devs);
        }
    }
}

pci_device_t *pci_find_device(unsigned int vendor, unsigned int device) {
    int i;
    list_head_t *p;
    pci_device_t *pci = 0;

    list_for_each(p, &pci_devs) {
        pci = list_entry(p, pci_device_t, list);

        if (pci->vendor == vendor && pci->device == device) {
            return pci;
        }
    }

    return 0;
}

pci_device_t *pci_find_device_by_classcode(unsigned int classcode) {
    int i;
    list_head_t *p;
    pci_device_t *pci = 0;

    list_for_each(p, &pci_devs) {
        pci = list_entry(p, pci_device_t, list);

        if (pci->classcode == classcode) return pci;
    }

    return 0;
}

const char *pci_intr_pin(int pin) {
    switch (pin) {
    case 0:
        return "NONE#";
    case 1:
        return "INTA#";
    case 2:
        return "INTB#";
    case 3:
        return "INTC#";
    case 4:
        return "INTD#";
    default:
        return "ERRPIN";
    }
}

void dump_pci_dev() {
    list_head_t *p;
    int i;

    list_for_each(p, &pci_devs) {
        pci_device_t *pci = list_entry(p, pci_device_t, list);
        // printk("vendor %04x device %04x class %04x:%02x bus %d intr %3d ", pci->vendor, pci->device, pci->classcode,
        //        pci->progif, pci->bus, pci->intr_line);
        // printk("%s\n", pci_get_info(pci->classcode, pci->progif));
        printk("PCI %03d:%02d.%d %02X %s%02d %04X%02X %04X:%04X %s\n", pci->bus, pci->dev, pci->devfn, pci->progif,
               pci_intr_pin(pci->intr_pin), pci->intr_line, pci->classcode, pci->progif, pci->vendor, pci->device,
               pci_get_info(pci->classcode, pci->progif));
#if 0
        switch (pci->hdr_type) {
        case PCI_HDRTYPE_NORMAL:
            printk("Normal Device\n");
            break;
        case PCI_HDRTYPE_BRIDGE:
            printk("PCI-PCI Bridge\n");
            break;
        case PCI_HDRTYPE_CARDBUS:
            printk("PCI-CardBus\n");
            break;
        default:
            printk("Not Support!\n");
            break;
        }
#else
        // for (int bar_inx = 0; bar_inx < BARS_CNT; bar_inx++) {
        //     printk("%08x ", pci->bars[bar_inx]);
        // }
        // printk("\n");
#endif
    }
}

int probe_pci_bus() {
    int v;
    int cmd;
    int dev, devfn;

    // Check if The IO Address was Used...
    cmd = PCI_CMD(0, 0, 0, 0);
    v = pci_read_config_long(cmd);

    if (v == 0xFFFFFFFF || v == 0x00000000 || v == 0x0000FFFF || v == 0xFFFF0000) {
        goto err;
    }

    // Maybe it's just an ISA Device
    // So We Must Check if It is PCI...
    for (dev = 0; dev < 32; dev++) {
        for (devfn = 0; devfn < 8; devfn++) {
            cmd = PCI_CMD(0, dev, devfn, PCI_CLASSCODE);
            v = pci_read_config_word(cmd);

            if (v == PCI_CLASS_BRIDGE_HOST || v == PCI_CLASS_DISPLAY_VGA) {
                return 1;
            }

            cmd = PCI_CMD(0, dev, devfn, PCI_VENDORID);
            v = pci_read_config_word(cmd);

            if (v == PCI_VENDORID_INTEL || v == PCI_VENDORID_COMPAQ) {
                return 1;
            }
        }
    }

    return 1;
err:
    printk("Can not find PCI bus on your computer\n");

    return 0;
}

void setup_pci() {
    if (!probe_pci_bus()) {
        return;
    }

    for (int i = 0; i < 256; i++) {
        scan_pci_bus(i);
    }

    dump_pci_dev();


    {
        int cmd = PCI_CMD(0, 31, 0, 0xF0);
        uint32_t rcba = pci_read_config_long(cmd);
        printk("IO-APIC RCBA: %08x\n", rcba); // 如果读到0xFFFFFFFF其实就代表机器太老了
    }


    // while (1)
    //     ;
}

typedef struct pci_info {
    unsigned long code;
    unsigned int flag;
    const char *info;
    const char *detail;
} pci_info_t;

pci_info_t pci_info[] = {
    {0x000000, 0, "VGA-Compatible devices", "Any device except for VGA-Compatible devices"},
    {0x000100, 0, "VGA-Compatible device", "VGA-Compatible Device"},
    {0x010000, 0, "SCSI Bus Controller", "SCSI Bus Controller"},
    {0x010100, 0, "IDE Controller", "ISA Compatibility mode-only controller"},
    {0x010105, 0, "IDE Controller", "PCI native mode-only controller"},
    {0x01010A, 0, "IDE Controller",
     "ISA Compatibility mode controller, supports both channels switched to PCI native mode"},
    {0x01010F, 0, "IDE Controller",
     "PCI native mode controller, supports both channels switched to ISA compatibility mode"},
    {0x010180, 0, "IDE Controller", "ISA Compatibility mode-only controller, supports bus mastering"},
    {0x010185, 0, "IDE Controller", "PCI native mode-only controller, supports bus mastering"},
    {0x01018A, 0, "IDE Controller",
     "ISA Compatibility mode controller, supports both channels switched to PCI native mode, supports bus mastering"},
    {0x01018F, 0, "IDE Controller",
     "PCI native mode controller, supports both channels switched to ISA compatibility mode, supports bus mastering"},
    {0x010200, 0, "Floppy Disk Controller", "Floppy Disk Controller"},
    {0x010300, 0, "IPI Bus Controller", "IPI Bus Controller"},
    {0x010400, 0, "RAID Controller", "RAID Controller"},
    {0x010520, 0, "ATA Controller", "ATA Controller (Single DMA)"},
    {0x010530, 0, "ATA Controller", "ATA Controller (Chained DMA)"},
    {0x010600, 0, "Serial ATA", "Vendor Specific Interface"},
    {0x010601, 0, "Serial ATA", "AHCI 1.0"},
    {0x010602, 0, "Serial ATA", "Serial Storage Bus"},
    {0x010700, 0, "SCSI", "Serial Attached SCSI (SAS)"},
    {0x010801, 0, "NVM Controller", "NVMHCI"},
    {0x010802, 0, "NVM Controller", "NVM Express"},
    {0x018000, 0, "Storage Controller", "Other Mass Storage Controller"},
    {0x020000, 0, "Ethernet Controller", "Ethernet Controller"},
    {0x020100, 0, "Token Ring Controller", "Token Ring Controller"},
    {0x020200, 0, "FDDI Controller", "FDDI Controller"},
    {0x020300, 0, "ATM Controller", "ATM Controller"},
    {0x020400, 0, "ISDN Controller", "ISDN Controller"},
    {0x020500, 0, "WorldFip Controller", "WorldFip Controller"},
    {0x0206, 1, "PICMG 2.14", "PICMG 2.14 Multi Computing"},
    {0x028000, 0, "Network Controller", "Other Network Controller"},
    {0x030000, 0, "VGA-Compatible Controller", "VGA-Compatible Controller"},
    {0x030001, 0, "8512-Compatible Controller", "8512-Compatible Controller"},
    {0x030100, 0, "XGA Controller", "XGA Controller"},
    {0x030200, 0, "3D Controller (Not VGA-Compatible)", "3D Controller (Not VGA-Compatible)"},
    {0x038000, 0, "Display Controller", "Other Display Controller"},
    {0x040000, 0, "Video Device", "Video Device"},
    {0x040100, 0, "Audio Device", "Audio Device"},
    {0x040200, 0, "Computer Telephony Device", "Computer Telephony Device"},
    {0x048000, 0, "Other Multimedia Device", "Other Multimedia Device"},
    {0x050000, 0, "RAM Controller", "RAM Controller"},
    {0x050100, 0, "Flash Controller", "Flash Controller"},
    {0x058000, 0, "Memory Controller", "Other Memory Controller"},
    {0x060000, 0, "Host/PCI Bridge", "Host/PCI Bridge"},
    {0x060100, 0, "PCI/ISA Bridge", "PCI/ISA Bridge"},
    {0x060200, 0, "PCI/EISA Bridge", "PCI/EISA Bridge"},
    {0x060300, 0, "PCI/MCA Bridge", "PCI/Micro Channel bridge"},
    {0x060400, 0, "PCI-to-PCI Bridge", "PCI-to-PCI Bridge"},
    {0x060401, 0, "PCI-to-PCI Bridge", "PCI-to-PCI Bridge (Subtractive Decode)"},
    {0x060500, 0, "PCMCIA Bridge", "PCMCIA Bridge"},
    {0x060600, 0, "NuBus Bridge", "NuBus Bridge"},
    {0x060700, 0, "CardBus Bridge", "CardBus Bridge"},
    {0x060800, 0, "RACEway Bridge", "Transparent Mode"},
    {0x060801, 0, "RACEway Bridge", "Endpoint Mode"},
    {0x060940, 0, "PCI-to-PCI Bridge", "Semi-Transparent, Primary bus towards host CPU"},
    {0x060980, 0, "PCI-to-PCI Bridge", "Semi-Transparent, Secondary bus towards host CPU"},
    {0x060A, 1, "InfiniBrand-to-PCI Host Bridge", "InfiniBrand-to-PCI Host Bridge"},
    {0x0680, 1, "Other Bridge Device", "Other Bridge Device"},
    {0x070000, 0, "Serial Controller", " 8250-Generic XT-Compatible Serial Controller"},
    {0x070001, 0, "Serial Controller", "16450-Compatible Serial Controller"},
    {0x070002, 0, "Serial Controller", "16550-Compatible Serial Controller"},
    {0x070003, 0, "Serial Controller", "16650-Compatible Serial Controller"},
    {0x070004, 0, "Serial Controller", "16750-Compatible Serial Controller"},
    {0x070005, 0, "Serial Controller", "16850-Compatible Serial Controller"},
    {0x070006, 0, "Serial Controller", "16950-Compatible Serial Controller"},
    {0x070100, 0, "Parallel Port", "Parallel Port"},
    {0x070101, 0, "Parallel Port", "Bi-Directional Parallel Port"},
    {0x070102, 0, "X Parallel Port", "ECP 1.X Compliant Parallel Port"},
    {0x070103, 0, "IEEE 1284 Controller", "IEEE 1284 Controller"},
    {0x0701FE, 0, "IEEE 1284 Target Device", "IEEE 1284 Target Device"},
    {0x070200, 0, "Serial Controller", "Multiport Serial Controller"},
    {0x070300, 0, "Generic Modem", "Generic Modem"},
    {0x070301, 0, "Hayes Compatible Modem", "Hayes Compatible Modem (16450-Compatible Interface)"},
    {0x070302, 0, "Hayes Compatible Modem", "Hayes Compatible Modem (16550-Compatible Interface)"},
    {0x070303, 0, "Hayes Compatible Modem", "Hayes Compatible Modem (16650-Compatible Interface)"},
    {0x070304, 0, "Hayes Compatible Modem", "Hayes Compatible Modem (16750-Compatible Interface)"},
    {0x070400, 0, "IEEE 488.1/2 Controller", "IEEE 488.1/2 (GPIB) Controller"},
    {0x070500, 0, "Smart Card", "Smart Card"},
    {0x078000, 0, "Communications Device", "Other Communications Device"},
    {0x080000, 0, "Generic 8259 PIC", "Generic 8259 PIC"},
    {0x080001, 0, "ISA PIC", "ISA PIC"},
    {0x080002, 0, "EISA PIC", "EISA PIC"},
    {0x080010, 0, "APIC Interrupt Controller", "I/O APIC Interrupt Controller"},
    {0x080020, 0, "APIC Interrupt Controller", "I/O(x) APIC Interrupt Controller"},
    {0x080100, 0, "8237 DMA Controller", "Generic 8237 DMA Controller"},
    {0x080101, 0, "ISA DMA Controller", "ISA DMA Controller"},
    {0x080102, 0, "EISA DMA Controller", "EISA DMA Controller"},
    {0x080200, 0, "8254 System Timer", "Generic 8254 System Timer"},
    {0x080201, 0, "ISA System Timer", "ISA System Timer"},
    {0x080202, 0, "EISA System Timer", "EISA System Timer"},
    {0x080203, 0, "Timer", "HPET"},
    {0x080300, 0, "Generic RTC Controller", "Generic RTC Controller"},
    {0x080301, 0, "ISA RTC Controller", "ISA RTC Controller"},
    {0x080400, 0, "Generic PCI Hot-Plug Controller", "Generic PCI Hot-Plug Controller"},
    {0x088000, 0, "Other System Peripheral", "Other System Peripheral"},
    {0x0900, 1, "Keyboard Controller", "Keyboard Controller"},
    {0x0901, 1, "Digitizer Pen", "Digitizer"},
    {0x0902, 1, "Mouse Controller", "Mouse Controller"},
    {0x0903, 1, "Scanner Controller", "Scanner Controller"},
    {0x090400, 0, "Gameport Controller (Generic)", "Gameport Controller (Generic)"},
    {0x090410, 0, "Gameport Contrlller (Legacy)", "Gameport Contrlller (Legacy)"},
    {0x098000, 0, "Other Input Controller", "Other Input Controller"},
    {0x0A0000, 0, "Generic Docking Station", "Generic Docking Station"},
    {0x0A8000, 0, "Other Docking Station", "Other Docking Station"},
    {0x0B00, 1, "386 Processor", "386 Processor"},
    {0x0B01, 1, "486 Processor", "486 Processor"},
    {0x0B02, 1, "Pentium Processor", "Pentium Processor"},
    {0x0B10, 1, "Alpha Processor", "Alpha Processor"},
    {0x0B20, 1, "PowerPC Processor", "PowerPC Processor"},
    {0x0B30, 1, "MIPS Processor", "MIPS Processor"},
    {0x0B40, 1, "Co-Processor", "Co-Processor"},
    {0x0B80, 1, "Other Processor", "Other Processor"},
    {0x0C0000, 0, "FireWire IEEE 1394 Controller", "Generic"},
    {0x0C0010, 0, "FireWire IEEE 1394 Controller", "OHCI"},
    {0x0C01, 1, "ACCESS Bus Controller", "ACCESS Bus Controller"},
    {0x0C02, 1, "SSA", "SSA"},
    {0x0C0300, 0, "USB", "UHCI (Universal Host Controller Spec)"},
    {0x0C0310, 0, "USB", "OHCI (Open Host Controller Spec"},
    {0x0C0320, 0, "USB2 Host Controller", "EHCI (Intel Enhanced Host Controller Interface)"},
    {0x0C0330, 0, "USB3 Host Controller", "XHCI (USB3) Controller"},
    {0x0C0380, 0, "USB Controller", "Unspecified"},
    {0x0C03FE, 0, "USB Device", "USB (Not Host Controller)"},
    {0x0C04, 1, "Fibre Channel", "Fibre Channel"},
    {0x0C05, 1, "SMBus", "SMBus"},
    {0x0C0600, 0, "InfiniBand", "InfiniBand"},
    {0x0C0700, 0, "IPMI SMIC Interface", "IPMI SMIC Interface"},
    {0x0C0701, 0, "IPMI Kybd Interface", "IPMI Kybd Controller Style Interface"},
    {0x0C0702, 0, "IPMI Block Interface", "IPMI Block Transfer Interface"},
    {0x0C0800, 0, "SERCOS Interface", "SERCOS Interface Standard (IEC 61491)"},
    {0x0C0900, 0, "CANbus", "CANbus"},
    {0x0D00, 1, "iRDA Controller", "iRDA Compatible Controller"},
    {0x0D01, 1, "IR Controller", "Consumer IR Controller"},
    {0x0D10, 1, "RF Controller", "RF Controller"},
    {0x0D11, 1, "Bluetooth Controller", "Bluetooth Controller"},
    {0x0D12, 1, "Broadband Controller", "Broadband Controller"},
    {0x0D20, 1, "Ethernet Controller (802.11a)", "Ethernet Controller (802.11a)"},
    {0x0D21, 1, "Ethernet Controller (802.11b)", "Ethernet Controller (802.11b)"},
    {0x0D80, 1, "Wireless Controller", "Other Wireless Controller"},
    {0x0E00, 1, "I20 Architecture", "I20 Architecture"},
    {0x0E0000, 0, "Message FIFO", "Message FIFO"},
    {0x0F0100, 0, "TV Controller", "TV Controller"},
    {0x0F0200, 0, "Audio Controller", "Audio Controller"},
    {0x0F0300, 0, "Voice Controller", "Voice Controller"},
    {0x0F0400, 0, "Data Controller", "Data Controller"},
    {0x100000, 0, "Computing Encrpytion/Decryption", "Network and Computing Encrpytion/Decryption"},
    {0x101000, 0, "Entertainment Encryption/Decryption", "Entertainment Encryption/Decryption"},
    {0x108000, 0, "Other Encryption/Decryption", "Other Encryption/Decryption"},
    {0x110000, 0, "DPIO Modules", "DPIO Modules"},
    {0x110100, 0, "Performance Counters", "Performance Counters"},
    {0x111000, 0, "Communications Syncrhonization Plus Time and Frequency Test/Measurment",
     "Communications Syncrhonization Plus Time and Frequency Test/Measurment"},
    {0x112000, 0, "Management Card", "Management Card"},
    {0x118000, 0, "Acquisition/Signal Processing Controller", "Other Data Acquisition/Signal Processing Controller"},
    {0x000000, 0, 0}};

const char *pci_get_info(unsigned int classcode, unsigned int progif) {
    pci_info_t *p = pci_info;
    const char *s = 0;

    while (p->code != 0 || p->flag != 0 || p->info != 0) {
        unsigned long code = classcode;

        if (p->flag == 0) {
            code <<= 8;
            code |= progif & 0xFF;
        }

        if (p->code == code) {
            if (p->flag == 0) {
                return p->info;
            } else {
                s = p->info;
            }
        }

        p++;
    }

    return s;
}
