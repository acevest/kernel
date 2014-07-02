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
#include <types.h>
#include <system.h>
#include <printk.h>

LIST_HEAD(pci_devs);

const char *pci_get_info(unsigned int classcode, unsigned int progif);

int pci_read_config_byte(int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inb(PCI_DATA + (PCI_GET_CMD_REG(cmd) & 3));
}

int pci_read_config_word(int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inw(PCI_DATA + (PCI_GET_CMD_REG(cmd) & 2));
}

int pci_read_config_long(int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inl(PCI_DATA);
}

void pci_write_config_byte(int value, int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    outb(value & 0xFF, PCI_DATA);
}

void pci_write_config_word(int value, int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    outw(value & 0xFFFF, PCI_DATA);
}

void pci_write_config_long(int value, int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    outl(value, PCI_DATA);
}

void scan_pci_bus(int bus)
{
    u8 dev, devfn;
    u32 cmd;
    u32 v;
    int i;
    printk("scanning PCI bus %d\n", bus);
    
    for(dev=0; dev<32; dev++)
    {
        for(devfn =0; devfn<8; devfn++)
        {
            cmd = PCI_CMD(bus, dev, devfn, PCI_VENDORID);
            v = pci_read_config_word(cmd);
            //v = pci_read_config_long(cmd);
            if(v == 0xFFFF)
                continue;

#if 0
            printk("dev %d ", dev);
            unsigned int i;
            for(i=0; i<16; ++i)
            {
                cmd = PCI_CMD(bus, dev, devfn, i*4);
                printk("%08x ", pci_read_config_long(cmd));
            }

            printk("\n");
#endif

            pci_device_t *pci = kmalloc(sizeof(pci_device_t), 0);
            if(0 == pci)
            {
                printk("no space to alloc for pci_device_t\n");
                continue;
            }

            pci->bus    = bus;
            pci->dev    = dev;
            pci->devfn  = devfn;

            pci->vendor = v;

            cmd = PCI_CMD(bus, dev, devfn, PCI_DEVICEID);
            pci->device = pci_read_config_word(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_REVISION);
            pci->revision = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_PROGIF);
            pci->progif = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_CLASSCODE);
            pci->classcode = pci_read_config_word(cmd);
            
            cmd = PCI_CMD(bus, dev, devfn, PCI_INTRLINE);
            pci->intr_line = pci_read_config_byte(cmd);

            cmd = PCI_CMD(bus, dev, devfn, PCI_INTRPIN);
            pci->intr_pin = pci_read_config_byte(cmd);


            cmd = PCI_CMD(bus, dev, devfn, PCI_HDRTYPE);
            pci->hdr_type = pci_read_config_byte(cmd);
            pci->hdr_type &= PCI_HDRTYPE_MASK;

            for(i=0; i<BARS_CNT; ++i)
            {
                cmd = PCI_CMD(bus, dev, devfn, PCI_BAR0 + i*4);
                pci->bars[i] = pci_read_config_long(cmd);
            }

            list_add(&pci->list, &pci_devs);
        }
    }
}


pci_device_t *pci_find_device(unsigned int vendor, unsigned int device)
{
    int i;
    list_head_t *p;
    pci_device_t *pci = 0;

    list_for_each(p, &pci_devs)
    {
        pci = list_entry(p, pci_device_t, list);

        if(pci->vendor == vendor && pci->device == device)
            return pci;
    }


    return 0;
}

pci_device_t *pci_find_device_by_classcode(unsigned int classcode)
{
    int i;
    list_head_t *p;
    pci_device_t *pci = 0;

    list_for_each(p, &pci_devs)
    {
        pci = list_entry(p, pci_device_t, list);

        if(pci->classcode == classcode)
            return pci;
    }

    return 0;
}

void dump_pci_dev()
{
    list_head_t *p;
    int i;

    list_for_each(p, &pci_devs)
    {
        pci_device_t *pci = list_entry(p, pci_device_t, list);
        printk("Vendor %04x Device %04x Class %04x Intr %02d ", pci->vendor, pci->device, pci->classcode, pci->intr_line);
        printk("%s\n", pci_get_info(pci->classcode, pci->progif));
        continue;
        switch(pci->hdr_type)
        {
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
    }
}

int probe_pci_bus()
{
    int v;
    int cmd;
    int dev,devfn;

    // Check if The IO Address was Used...
    cmd = PCI_CMD(0,0,0,0);
    v = pci_read_config_long(cmd);

    if( v == 0xFFFFFFFF || v == 0x00000000 || v == 0x0000FFFF || v == 0xFFFF0000)
        goto err;

    // Maybe it's just an ISA Device
    // So We Must Check if It is PCI...
    for(dev=0; dev<32; dev++)
    {
        for(devfn=0; devfn<8; devfn++)
        {
            cmd = PCI_CMD(0, dev, devfn, PCI_CLASSCODE);
            v = pci_read_config_word(cmd);

            if(v == PCI_CLASS_BRIDGE_HOST || v == PCI_CLASS_DISPLAY_VGA)
                return 1;

            cmd = PCI_CMD(0, dev, devfn, PCI_VENDORID);
            v = pci_read_config_word(cmd);

            if(v == PCI_VENDORID_INTEL || v == PCI_VENDORID_COMPAQ)
                return 1;
        }
    }

    return 1;
err:
    printk("Can not find PCI bus on your computer\n");

    return 0;
}

void    setup_pci()
{
    if(!probe_pci_bus())
        return ;

    scan_pci_bus(0);

    dump_pci_dev();
}

typedef struct pci_info {
    unsigned long code;
    unsigned int  flag;
    const char   *info;
} pci_info_t;

pci_info_t pci_info[] = {
    { 0x000000, 0, "Any device except for VGA-Compatible devices" },
    { 0x000100, 0, "VGA-Compatible Device" },
    { 0x010000, 0, "SCSI Bus Controller" },
    { 0x0101,   1, "IDE Controller" },
    { 0x010200, 0, "Floppy Disk Controller" },
    { 0x010300, 0, "IPI Bus Controller" },
    { 0x010400, 0, "RAID Controller" },
    { 0x010520, 0, "ATA Controller (Single DMA)" },
    { 0x010530, 0, "ATA Controller (Chained DMA)" },
    { 0x010600, 0, "Serial ATA (Vendor Specific Interface)" },
    { 0x010601, 0, "Serial ATA (AHCI 1.0)" },
    { 0x010700, 0, "Serial Attached SCSI (SAS)" },
    { 0x018000, 0, "Other Mass Storage Controller" },
    { 0x020000, 0, "Ethernet Controller" },
    { 0x020100, 0, "Token Ring Controller" },
    { 0x020200, 0, "FDDI Controller" },
    { 0x020300, 0, "ATM Controller" },
    { 0x020400, 0, "ISDN Controller" },
    { 0x020500, 0, "WorldFip Controller" },
    { 0x0206,   1, "PICMG 2.14 Multi Computing" },
    { 0x028000, 0, "Other Network Controller" },
    { 0x030000, 0, "VGA-Compatible Controller" },
    { 0x030001, 0, "8512-Compatible Controller" },
    { 0x030100, 0, "XGA Controller" },
    { 0x030200, 0, "3D Controller (Not VGA-Compatible)" },
    { 0x038000, 0, "Other Display Controller" },
    { 0x040000, 0, "Video Device" },
    { 0x040100, 0, "Audio Device" },
    { 0x040200, 0, "Computer Telephony Device" },
    { 0x048000, 0, "Other Multimedia Device" },
    { 0x050000, 0, "RAM Controller" },
    { 0x050100, 0, "Flash Controller" },
    { 0x058000, 0, "Other Memory Controller" },
    { 0x060000, 0, "Host Bridge" },
    { 0x060100, 0, "ISA Bridge" },
    { 0x060200, 0, "EISA Bridge" },
    { 0x060300, 0, "MCA Bridge" },
    { 0x060400, 0, "PCI-to-PCI Bridge" },
    { 0x060401, 0, "PCI-to-PCI Bridge (Subtractive Decode)" },
    { 0x060500, 0, "PCMCIA Bridge" },
    { 0x060600, 0, "NuBus Bridge" },
    { 0x060700, 0, "CardBus Bridge" },
    { 0x0608,   1, "RACEway Bridge" },
    { 0x060940, 0, "PCI-to-PCI Bridge (Semi-Transparent, Primary)" },
    { 0x060980, 0, "PCI-to-PCI Bridge (Semi-Transparent, Secondary)" },
    { 0x060A00, 0, "InfiniBrand-to-PCI Host Bridge" },
    { 0x068000, 0, "Other Bridge Device" },
    { 0x070000, 0, "Generic XT-Compatible Serial Controller" },
    { 0x070001, 0, "16450-Compatible Serial Controller" },
    { 0x070002, 0, "16550-Compatible Serial Controller" },
    { 0x070003, 0, "16650-Compatible Serial Controller" },
    { 0x070004, 0, "16750-Compatible Serial Controller" },
    { 0x070005, 0, "16850-Compatible Serial Controller" },
    { 0x070006, 0, "16950-Compatible Serial Controller" },
    { 0x070100, 0, "Parallel Port" },
    { 0x070101, 0, "Bi-Directional Parallel Port" },
    { 0x070102, 0, "ECP 1.X Compliant Parallel Port" },
    { 0x070103, 0, "IEEE 1284 Controller" },
    { 0x0701FE, 0, "IEEE 1284 Target Device" },
    { 0x070200, 0, "Multiport Serial Controller" },
    { 0x070300, 0, "Generic Modem" },
    { 0x070301, 0, "Hayes Compatible Modem (16450-Compatible Interface)" },
    { 0x070302, 0, "Hayes Compatible Modem (16550-Compatible Interface)" },
    { 0x070303, 0, "Hayes Compatible Modem (16650-Compatible Interface)" },
    { 0x070304, 0, "Hayes Compatible Modem (16750-Compatible Interface)" },
    { 0x070400, 0, "IEEE 488.1/2 (GPIB) Controller" },
    { 0x070500, 0, "Smart Card" },
    { 0x078000, 0, "Other Communications Device" },
    { 0x080000, 0, "Generic 8259 PIC" },
    { 0x080001, 0, "ISA PIC" },
    { 0x080002, 0, "EISA PIC" },
    { 0x080010, 0, "I/O APIC Interrupt Controller" },
    { 0x080020, 0, "I/O(x) APIC Interrupt Controller" },
    { 0x080100, 0, "Generic 8237 DMA Controller" },
    { 0x080101, 0, "ISA DMA Controller" },
    { 0x080102, 0, "EISA DMA Controller" },
    { 0x080200, 0, "Generic 8254 System Timer" },
    { 0x080201, 0, "ISA System Timer" },
    { 0x080202, 0, "EISA System Timer" },
    { 0x080300, 0, "Generic RTC Controller" },
    { 0x080301, 0, "ISA RTC Controller" },
    { 0x080400, 0, "Generic PCI Hot-Plug Controller" },
    { 0x088000, 0, "Other System Peripheral" },
    { 0x090000, 0, "Keyboard Controller" },
    { 0x090100, 0, "Digitizer" },
    { 0x090200, 0, "Mouse Controller" },
    { 0x090300, 0, "Scanner Controller" },
    { 0x090400, 0, "Gameport Controller (Generic)" },
    { 0x090410, 0, "Gameport Contrlller (Legacy)" },
    { 0x098000, 0, "Other Input Controller" },
    { 0x0A0000, 0, "Generic Docking Station" },
    { 0x0A8000, 0, "Other Docking Station" },
    { 0x0B0000, 0, "386 Processor" },
    { 0x0B0100, 0, "486 Processor" },
    { 0x0B0200, 0, "Pentium Processor" },
    { 0x0B1000, 0, "Alpha Processor" },
    { 0x0B2000, 0, "PowerPC Processor" },
    { 0x0B3000, 0, "MIPS Processor" },
    { 0x0B4000, 0, "Co-Processor" },
    { 0x0C0000, 0, "IEEE 1394 Controller (FireWire)" },
    { 0x0C0010, 0, "IEEE 1394 Controller (1394 OpenHCI Spec)" },
    { 0x0C0100, 0, "ACCESS.bus" },
    { 0x0C0200, 0, "SSA" },
    { 0x0C0300, 0, "USB (Universal Host Controller Spec)" },
    { 0x0C0310, 0, "USB (Open Host Controller Spec" },
    { 0x0C0320, 0, "USB2 Host Controller (Intel Enhanced Host Controller Interface)" },
    { 0x0C0380, 0, "USB" },
    { 0x0C03FE, 0, "USB (Not Host Controller)" },
    { 0x0C0400, 0, "Fibre Channel" },
    { 0x0C0500, 0, "SMBus" },
    { 0x0C0600, 0, "InfiniBand" },
    { 0x0C0700, 0, "IPMI SMIC Interface" },
    { 0x0C0701, 0, "IPMI Kybd Controller Style Interface" },
    { 0x0C0702, 0, "IPMI Block Transfer Interface" },
    { 0x0C0800, 0, "SERCOS Interface Standard (IEC 61491)" },
    { 0x0C0900, 0, "CANbus" },
    { 0x0D0000, 0, "iRDA Compatible Controller" },
    { 0x0D0100, 0, "Consumer IR Controller" },
    { 0x0D1000, 0, "RF Controller" },
    { 0x0D1100, 0, "Bluetooth Controller" },
    { 0x0D1200, 0, "Broadband Controller" },
    { 0x0D2000, 0, "Ethernet Controller (802.11a)" },
    { 0x0D2100, 0, "Ethernet Controller (802.11b)" },
    { 0x0D8000, 0, "Other Wireless Controller" },
    { 0x0E00,   1, "I20 Architecture" },
    { 0x0E0000, 0, "Message FIFO" },
    { 0x0F0100, 0, "TV Controller" },
    { 0x0F0200, 0, "Audio Controller" },
    { 0x0F0300, 0, "Voice Controller" },
    { 0x0F0400, 0, "Data Controller" },
    { 0x100000, 0, "Network and Computing Encrpytion/Decryption" },
    { 0x101000, 0, "Entertainment Encryption/Decryption" },
    { 0x108000, 0, "Other Encryption/Decryption" },
    { 0x110000, 0, "DPIO Modules" },
    { 0x110100, 0, "Performance Counters" },
    { 0x111000, 0, "Communications Syncrhonization Plus Time and Frequency Test/Measurment" },
    { 0x112000, 0, "Management Card" },
    { 0x118000, 0, "Other Data Acquisition/Signal Processing Controller" },
    { 0x000000, 0, 0 }
};

const char *pci_get_info(unsigned int classcode, unsigned int progif)
{
    pci_info_t *p = pci_info;
    const char *s = 0;

    while(p->code != 0 || p->flag != 0 || p->info != 0)
    {
        unsigned long code = classcode;

        if(p->flag == 0)
        {
            code <<= 8;
            code |= progif & 0xFF;
        }

        if(p->code == code)
        {
            if(p->flag == 0)
            {
                return p->info;
            }
            else
            {
                s = p->info;
            }
        }

        p++;
    }

    return s;
}
