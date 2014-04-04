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

void ScanPCIBus(int bus);
int ProbePCIBus();

static int pci_read_config_byte(int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inb(PCI_DATA + (PCI_GET_CMD_REG(cmd) & 3));
}

static int pci_read_config_word(int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inw(PCI_DATA + (PCI_GET_CMD_REG(cmd) & 2));
}

static int pci_read_config_long(int cmd)
{
    outl(PCI_CONFIG_CMD(cmd), PCI_ADDR);
    return inl(PCI_DATA);
}


void    setup_pci()
{
    if(!ProbePCIBus())
        return ;

    ScanPCIBus(0);
}

void ScanPCIBus(int bus)
{
    u8 dev, devfn;
    u32 cmd;
    u32 retval;
    printk("Scanning PCI Bus.\n");
    
    for(dev=0; dev<32; dev++)
        for(devfn =0; devfn<8; devfn++)
        {
            cmd = PCI_CMD(bus, dev, devfn, PCI_VENDORID);
            retval = pci_read_config_word(cmd);
            if(retval == 0xFFFF)
                continue;
            //Vendor
            printk("%x ", retval);

            //Device
            cmd = PCI_CMD(bus, dev, devfn, PCI_DEVICEID);
            retval = pci_read_config_word(cmd);
            printk("%x ", retval);
        
            // Class Device
            cmd = PCI_CMD(bus, dev, devfn, PCI_CLASSPROG);
            retval = pci_read_config_byte(cmd);
            printk("%x ", retval);

            cmd = PCI_CMD(bus, dev, devfn, PCI_CLASSDEVICE);
            retval = pci_read_config_word(cmd);
            printk("%x ", retval);
            

            //Interrupt Line
            cmd = PCI_CMD(bus, dev, devfn, PCI_INTRLINE);
            retval = pci_read_config_byte(cmd);
            printk("%x ", retval);

            //Interrupt Pin
            cmd = PCI_CMD(bus, dev, devfn, PCI_INTRPIN);
            retval = pci_read_config_byte(cmd);
            printk("%x ", retval);

            cmd = PCI_CMD(bus, dev, devfn, PCI_HDRTYPE);
            retval = pci_read_config_byte(cmd);
            printk("%x ", retval);
            retval &= PCI_HDRTYPE_MASK;
            printk("%x", retval);
            
            switch(retval)
            {
            case PCI_HDRTYPE_NORMAL:
                printk("Normal Device\n");
                break;
            case PCI_HDRTYPE_BRIDGE:
                printk("Aha PCI-PCI Bridge\n");
                break;
            case PCI_HDRTYPE_CARDBUS:
                printk("Wow PCI-CardBus\n");
                break;
            default:
                printk("Not Support!\n");
                break;
            }
        }
}

int ProbePCIBus()
{
    int retval;
    int cmd;
    int dev,devfn;
    char errmsg[]="Can not find PCI bus on your computer";

    // Check if The IO Address was Used...
    cmd = PCI_CMD(0,0,0,0);
    retval = pci_read_config_long(cmd);
    if( retval == 0xFFFFFFFF || retval == 0x00000000
    ||  retval == 0x0000FFFF || retval == 0xFFFF0000)
    {
        printk("%s\n",errmsg);
        return 0;
    }

    // Maybe it's just an ISA Device
    // So We Must Check if It is PCI...
    for(dev=0; dev<32; dev++)
        for(devfn=0; devfn<8; devfn++)
        {
            cmd = PCI_CMD(0, dev, devfn, PCI_CLASSDEVICE);
            retval = pci_read_config_word(cmd);
            if(retval == PCI_CLASS_BRIDGE_HOST
            || retval == PCI_CLASS_DISPLAY_VGA)
                return 1;
            cmd = PCI_CMD(0, dev, devfn, PCI_VENDORID);
            retval = pci_read_config_word(cmd);
            if(retval == PCI_VENDORID_INTEL
            || retval == PCI_VENDORID_COMPAQ)
                return 1;
        }


    printk("%s\n",errmsg);
    return 0;
}
