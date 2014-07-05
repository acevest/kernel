/*
 *--------------------------------------------------------------------------
 *   File Name: fs.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Fri Feb 12 20:48:50 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <fs.h>
#include <io.h>
#include <printk.h>
#include <system.h>
#define SECT_SIZE    512

File file_table[NR_FILES] __attribute__ ((__aligned__(PAGE_SIZE))) = {{0,},};
extern unsigned int ext2_start_sect;
void hd_read(dev_t dev, u64 sect_nr, void *buf, u32 count);
void init_file_table();
void save_boot_part(int n, pPartition p, u32 base_sect);
void read_ext_part(u32 base, u32 offset);

void setup_fs()
{
    int i, minor;
    pPartition p;
    unsigned char *buf;


    init_file_table();


    minor = DEV_MINOR(ROOT_DEV);
    buf = (unsigned char *) get_virt_pages(1);
    hd_read(ROOT_DEV, 0, buf, SECT_SIZE);


    for(i=0; i<4; i++)
    {
        p = (pPartition)(buf+PARTS_POS) + i;
        if(p->Type == 0)
            continue;

        //save_boot_part(i+1, p, p->AbsoluteSectNo);
        save_boot_part(i, p, p->AbsoluteSectNo);

        printk("hd%d\tbase: %08x size: %08x type:%02x",
            i, p->AbsoluteSectNo, p->PartitionSize, p->Type);
        if(p->Type == 0x05)
        {
            printk("\tExtend\n");
            //read_ext_part(p->AbsoluteSectNo, 0);
        }
        else
            printk("\n");
    }


    printk("ext2_start_sect: %x\n", ext2_start_sect);


    return ;
}

void init_file_table()
{
    int i;

    for(i=0; i<NR_FILES; i++)
    {
        file_table[i].count    = 0;
        file_table[i].ino_nr    = 0;
        file_table[i].inode    = NULL;
    }
}

void save_boot_part(int n, pPartition p, u32 base_sect)
{
    if(p->Type == 0x05)
        panic("partition should not be extended");

    int minor = DEV_MINOR(ROOT_DEV);

    if(minor-1 == n)
        ext2_start_sect = base_sect;
}

static unsigned int ext_part = 5;
void read_ext_part(u32 base, u32 offset)
{
    unsigned char *buf;
    pPartition p;
    buf = get_virt_pages(1);

    //printk("^^^^^^^^^:%08x\n", base+offset);
    hd_read(ROOT_DEV, base+offset, buf, SECT_SIZE);
    int i;

    for(i=0; i<4; i++)
    {
        p = (pPartition)(buf+PARTS_POS) + i;
        if(p->Type == 0x00)
            continue;

        if(p->Type != 0x05)
        {

            //save_boot_part(ext_part, p, 0);
            printk("  hd%d\tbase: %08x size: %08x type:%02x\n",
                ext_part++, base+p->AbsoluteSectNo,
                p->PartitionSize, p->Type);
        }
        else
        {
            read_ext_part(base, p->AbsoluteSectNo);
        }
    }

    printk("\n");

    free_virt_pages(buf);
}
