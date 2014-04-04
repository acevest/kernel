/*
 *--------------------------------------------------------------------------
 *   File Name: boot.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Wed Dec 30 21:55:29 2009
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */


#include <system.h>
#include <boot/boot.h>
#include <page.h>
#include <bits.h>
#include <assert.h>

extern void    parse_cmdline(const char *cmdline);
void    init_free_area(u32 base, u32 len);


struct boot_params boot_params __attribute__((aligned(32)));

void init_boot_params(multiboot_info_t *p)
{
    boot_params.cmdline = (char *) p->cmdline;

    // KB to Bytes
    // no need to concern about 64bit
    boot_params.mem_lower = p->mem_lower << 10;
    boot_params.mem_upper = p->mem_upper << 10;

    boot_params.boot_device = p->boot_device;

    memory_map_t *mmap = (memory_map_t *) pa2va(p->mmap_addr);

    unsigned int i;
    boot_params.e820map.map_cnt = p->mmap_length / sizeof(memory_map_t);
    for(i=0; i<boot_params.e820map.map_cnt; ++i, ++mmap)
    {
        boot_params.e820map.map[i].addr = mmap->base_addr_low;
        boot_params.e820map.map[i].size = mmap->length_low;
        boot_params.e820map.map[i].type = mmap->type;
    }
}

void CheckKernel(unsigned long addr, unsigned long magic)
{
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        printk("Your boot loader does not support multiboot.\n");
        while(1);
    }

    multiboot_info_t *mbi = (multiboot_info_t *) addr;

    if( (mbi->flags & 0x47) != 0x47)
    {
        printk("Kernel Need More Information\n");
        while(1);
    }

    init_boot_params(mbi);

}

#if 0
{
    int i, mmapCount;
    pmmapItem pMPI;

    pMultiBootInfo pMBI = (pMultiBootInfo) addr;


    
    {
        printk("mmap_addr %x mmap_length %x\n", pMBI->mmap_addr, pMBI->mmap_length);
        while(1);

    }


    system.boot_device    = pMBI->boot_device;

    /* 分析命令行参数 */
    parse_cmdline((const char *)pMBI->cmdline);

    system.mmap_addr= pMBI->mmap_addr;
    system.mmap_size= pMBI->mmap_length;
    system.mm_lower    = pMBI->mem_lower;
    system.mm_upper    = pMBI->mem_upper;
    system.mm_size        = 0x100000 + (system.mm_upper<<10);
    // 最大只管理1G的内存
    if(system.mm_size > MAX_SUPT_PHYMM_SIZE)
        system.mm_size = MAX_SUPT_PHYMM_SIZE;

    // 重新进行页映射
    system.page_count    = (system.mm_size >> PAGE_SHIFT);
    system.pgd        = (unsigned long *) &krnl_end;
    system.pte_start    = system.pgd + PAGE_ITEMS;

    unsigned long *pde = system.pgd;
    unsigned long *pte = system.pte_start;
    unsigned long pde_count = system.page_count/PAGE_ITEMS + 
                  (system.page_count%PAGE_ITEMS != 0);

    unsigned long pde_base = KRNLADDR>>22;
    pde[pde_base] = 7 + va2pa(pte);
    for(i=pde_base+1; i<(pde_base+pde_count); i++)
        pde[i] = pde[i-1] + 0x1000;
    pte[0] = 7;
    for(i=1; i<system.page_count; i++)
        pte[i] = pte[i-1] + 0x1000;

    //asm("xchg %bx,%bx");
    asm("movl %%edx, %%cr3"::"d"(va2pa(pde)));

    system.pte_end        = pte+i;
    system.pte_end        = (u32*) PAGE_UP(system.pte_end);

    system.page_map        = (pPage)system.pte_end;
    system.page_bitmap    = PAGE_UP((system.page_map +
                    system.page_count));
    memset((u32)system.page_map, 0,
        system.page_bitmap-(u32)system.page_map);

    // 初始化伙伴系统的位图
    unsigned int    bmSize = system.page_count>>1;//bitmap size
    u32        bmaddr = system.page_bitmap;
    for(i=0; i<MAX_ORDER; i++)
    {
        freeArea[i].map = (unsigned char *)bmaddr;
        bmSize = (bmSize>>1) + (bmSize%8 != 0);
        bmaddr += bmSize;
        bmaddr = ALIGN(bmaddr, sizeof(long));
    }
    memset(system.page_bitmap, 0x00, bmaddr - system.page_bitmap);

    system.kernel_end    = PAGE_UP(bmaddr);
    


    mmapCount = system.mmap_size/sizeof(mmapItem);
    pMPI    = (pmmapItem) system.mmap_addr;

#if 1
    printk("mm_size: %d MB\n", system.mm_size>>20);
    printk("page_count:%d page_map:%08x page_bitmap:%08x\n",
        system.page_count, system.page_map, system.page_bitmap);
    printk("boot device: %x\n", (unsigned int)system.boot_device);
    printk("CmdLine: %s\n", system.cmdline);
    printk("mmap count: %d\n", mmapCount);
#endif


    // 初始化空闲页链表头
    for(i=0; i<MAX_ORDER; i++)
        INIT_LIST_HEAD(&freeArea[i].freeList);

    // 初始化描述每个页的结构体
    for(i=0; i<system.page_count; i++)
    {
        INIT_LIST_HEAD(&pgmap[i].list);
        pgmap[i].mapNR = i;
        pgmap[i].count = 0;
    }

    for(i=0; i<mmapCount; i++, pMPI++)
    {
        // unsupport high part
        u32 base    = pMPI->base_addr_low;
        u32 length    = pMPI->length_low;
        u32 type    = pMPI->type;


        printk("--%08x %08x %02x\n", base, length, type);


        if(type == E820_RAM)
        {
            if((base+length)>MAX_SUPT_PHYMM_SIZE)
            {
                length = MAX_SUPT_PHYMM_SIZE - base;
            }

            if(base < va2pa(system.kernel_end)
            && va2pa(system.kernel_end) < base+length)
            {
                unsigned int offset;
                offset = va2pa(PAGE_UP(system.kernel_end)) - base;
                base += offset;
                length -= offset;
            }
            if(base == 0)
            {
                if(length < 0x1000)
                    continue;
                base += 0x1000;
                length -= 0x1000;
            }
            printk("base:%08x length:%08x addr:%08x\n",
            base, length, base+length);

            init_free_area(base, length);
        }
    }
}

int get_order(unsigned int min_pfn, unsigned int max_pfn)
{
    int i, order, size;

    assert(min_pfn<=max_pfn);
    for(i=order=0; i<MAX_ORDER; i++)
    {
        if(min_pfn % (1UL<<i) == 0)
            order = i;
    }

    size = 1UL<<order;

    while(min_pfn+size>max_pfn)
    {
        order--;
        size >>= 1;
    }

    return order;
}

void    init_free_area(u32 base, u32 len)
{
    unsigned int max_pfn, min_pfn;
    unsigned int i,order,size;

    min_pfn = get_pfn(base);
    max_pfn = get_pfn(base+len);
/*
    printk("%08x\t%08x\t%x\t%x\t%x\t%d\n",base, base+len, 
                    min_pfn, max_pfn,
                    max_pfn-min_pfn,
                    max_pfn-min_pfn);
*/
    assert(min_pfn<=system.page_count);
    assert(max_pfn<=system.page_count);

    while(min_pfn != max_pfn)
    {
        order = get_order(min_pfn, max_pfn);
        //printk("min:%05d end:%05d max:%05d order:%02d\n",
        //    min_pfn, min_pfn+(1<<order), max_pfn, order);

        //order = (order>=MAX_ORDER)? MAX_ORDER-1: order;
        assert(0<=order && order<MAX_ORDER);

        size = 1<<order;

        pgmap[min_pfn].order = order;
        list_add(&pgmap[min_pfn].list, &freeArea[order].freeList);

        change_bit(pgmap[min_pfn].mapNR>>(order+1),
                (unsigned long *)freeArea[order].map);

        //printk("%d\t%08x %d\t",pgmap[min_pfn].mapNR>>(order+1),
        //            freeArea[order].map, min_pfn);
        min_pfn += size;
        //printk("%d\t%d\t%d\t%d\n",min_pfn, max_pfn, order, size);
        //int j=3000000;while(j--);
    }
}
#endif
