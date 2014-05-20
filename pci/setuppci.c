/*
 *--------------------------------------------------------------------------
 *   File Name: setuppci.c
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
    outb(value & 0xFFFF, PCI_DATA);
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

#if 0
            if(pci->hdr_type == PCI_HDRTYPE_BRIDGE)
            {
                cmd = PCI_CMD(bus, dev, devfn, PCI_PRIMARY_BUS_NUMBER);
                pci->primary_bus_nr = pci_read_config_byte(cmd);

                cmd = PCI_CMD(bus, dev, devfn, PCI_SECONDARY_BUS_NUMBER);
                pci->secondary_bus_nr = pci_read_config_byte(cmd);

                printk("    PCI BUS: %d %d\n", pci->primary_bus_nr, pci->secondary_bus_nr);

                scan_pci_bus(pci->secondary_bus_nr);
            }
#endif

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

void dump_pci_dev()
{
    list_head_t *p;
    int i;

    list_for_each(p, &pci_devs)
    {
        pci_device_t *pci = list_entry(p, pci_device_t, list);
        printk("Vendor %x Device %x Class %x Revision %x IntrLine %d Pin %d ", pci->vendor, pci->device, pci->classcode, pci->revision, pci->intr_line, pci->intr_pin);
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

