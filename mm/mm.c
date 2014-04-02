/*
 *--------------------------------------------------------------------------
 *   File Name:	mm.c
 * 
 * Description:	none
 * 
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:	1.0
 * Create Date: Wed Mar  4 21:08:47 2009
 * Last Update: Wed Mar  4 21:08:47 2009
 * 
 *--------------------------------------------------------------------------
 */
#include<printk.h>
#include<system.h>
#include<page.h>
#include<types.h>
#include<bits.h>
#include<mm.h>
#include<boot/bootparam.h>


extern char kernel_begin, kernel_end;
extern char etext,edata,end;

static void e820_print_type(unsigned long type)
{
    switch (type) {
    case E820_RAM:
        printk("usable");
        break;
    case E820_RESERVED:
        printk("reserved");
        break;
    case E820_ACPI:
        printk("ACPI data");
        break;
    case E820_NVS:
        printk("ACPI NVS");
        break;
    case E820_UNUSABLE:
        printk("unusable");
        break;
    default:
        printk("type %x", type);
        break;
    }    
}


void e820_print_map()
{
    unsigned int i=0;

    for(i=0; i<boot_params.e820map.map_cnt; ++i)
    {
        struct e820_entry *p = boot_params.e820map.map + i;

		printk("[%02d] 0x%08x - 0x%08x size %- 10d %8dKB %5dMB ", i, p->addr, p->addr + p->size, p->size, p->size>>10, p->size>>20);

        e820_print_type(p->type);

		printk("\n");
    }
}


typedef struct bootmem_data {
    unsigned long min_pfn;
    unsigned long max_pfn;

    unsigned long last_offset;   // offset to pfn2pa(this->min_pfn);
    unsigned long last_hint_inx; // last hit index in bitmap

    void *bitmap;   
    unsigned long mapsize;
} bootmem_data_t;

bootmem_data_t bootmem_data;
#define pfn2inx(pfn) ((pfn) - bootmem_data.min_pfn)
#define inx2pfn(inx) ((inx) + bootmem_data.min_pfn)

void e820_init_bootmem_data()
{
    unsigned int i=0;

    memset(&bootmem_data, 0, sizeof(bootmem_data));
    bootmem_data.min_pfn    = ~0UL;

    unsigned long bgn_pfn;
    unsigned long end_pfn;

    for(i=0; i<boot_params.e820map.map_cnt; ++i)
    {
        struct e820_entry *p = boot_params.e820map.map + i;

        if(p->type != E820_RAM)
            continue;

        bgn_pfn = pa2pfn(p->addr);
        end_pfn = pa2pfn(p->addr + p->size);

        if(bootmem_data.min_pfn > bgn_pfn)
            bootmem_data.min_pfn = bgn_pfn;

        if(bootmem_data.max_pfn < end_pfn)
            bootmem_data.max_pfn = end_pfn;
    }

    // limit min_pfn
    unsigned long kernel_begin_pfn = va2pfn(&kernel_begin);
    if(bootmem_data.min_pfn < kernel_begin_pfn)
    {
        bootmem_data.min_pfn = kernel_begin_pfn;
    }

    // limit max_pfn
    unsigned long max_support_pfn = pa2pfn(MAX_SUPT_PHYMM_SIZE);
    if(bootmem_data.max_pfn > max_support_pfn)
    {
        bootmem_data.max_pfn = max_support_pfn;
    }

    if(bootmem_data.min_pfn >= bootmem_data.max_pfn)
    {
        printk("can not go on playing...\n");
        while(1);
    }
}

void register_bootmem_pages()
{
    unsigned int i=0;
    unsigned int j=0;

    for(i=0; i<boot_params.e820map.map_cnt; ++i)
    {
        struct e820_entry *p = boot_params.e820map.map + i;

        if(p->type != E820_RAM)
            continue;

        unsigned long bgn_pfn = PFN_UP(p->addr);
        unsigned long end_pfn = PFN_DW(p->addr + p->size);

        bgn_pfn = bgn_pfn > bootmem_data.min_pfn ? bgn_pfn : bootmem_data.min_pfn;
        end_pfn = end_pfn < bootmem_data.max_pfn ? end_pfn : bootmem_data.max_pfn;

        if(bgn_pfn >= end_pfn)
        {
            continue;
        }

        unsigned long bgn_inx = pfn2inx(bgn_pfn);
        unsigned long end_inx = pfn2inx(end_pfn);

        for(j=bgn_inx; j<end_inx; ++j)
        {
            test_and_clear_bit(j, bootmem_data.bitmap);
        }
    } 

}

void reserve_bootmem(unsigned long bgn_inx, unsigned long end_inx)
{
    printk("reserve %d %d\n", bgn_inx, end_inx);

    int i=0;
    for(i=bgn_inx; i<end_inx; ++i)
    {
        test_and_set_bit(i, bootmem_data.bitmap);
    }
}

void reserve_kernel_pages()
{
    reserve_bootmem(pfn2inx(PFN_DW(va2pa(&kernel_begin))), pfn2inx(PFN_UP(va2pa(&kernel_end))));
}

void reserve_bootmem_pages()
{
    unsigned long bgn_pfn = PFN_DW(va2pa(bootmem_data.bitmap));

    unsigned long end_pfn = bgn_pfn;

    end_pfn += PFN_UP(bootmem_data.mapsize);

    reserve_bootmem(pfn2inx(bgn_pfn), pfn2inx(end_pfn));
}

void init_bootmem_allocator()
{
    int mapsize = (bootmem_data.max_pfn - bootmem_data.min_pfn + 7) / 8;

    bootmem_data.bitmap = &kernel_end;
    bootmem_data.mapsize= mapsize;

    memset(bootmem_data.bitmap, 0xFF, mapsize);

    register_bootmem_pages();

    reserve_kernel_pages();

    reserve_bootmem_pages();
}

void init_bootmem()
{
    e820_print_map();
    e820_init_bootmem_data();
    init_bootmem_allocator();
    
    printk("alloc 10 bytes align 8    addr %08x\n", alloc_bootmem(10, 8));
    printk("alloc 40961 bytes align 4096 addr %08x\n", alloc_bootmem(40961, 4096));
    printk("alloc 5  bytes align 4    addr %08x\n", alloc_bootmem(5, 4));
    printk("alloc 10 bytes align 1024 addr %08x\n", alloc_bootmem(10, 1024));
    printk("alloc 123bytes align 2    addr %08x\n", alloc_bootmem(123, 2));
    printk("alloc 123bytes align 2    addr %08x\n", alloc_bootmem(123, 2));
}

unsigned long align_bootmem_index(unsigned long inx, unsigned long align)
{
    unsigned long min_pfn = bootmem_data.min_pfn;
    unsigned long aln_pfn = ALIGN(min_pfn + inx, align);

    return aln_pfn - min_pfn;
}

unsigned long align_bootmem_offset(unsigned long offset,  unsigned long align)
{
    unsigned long base = pfn2pa(bootmem_data.min_pfn);

    return ALIGN(base + offset, align) - base;
}

void *alloc_bootmem(unsigned long size, unsigned long align)
{
    bootmem_data_t *pbd = &bootmem_data;

    assert(size != 0);
    assert((align & (align-1)) == 0); // must be power of 2
    
    unsigned long fallback = 0;
    unsigned long bgn_inx, end_inx, step;

    step = align >> PAGE_SHIFT;
    step = step > 0 ? step : 1;

    bgn_inx = ALIGN(pbd->min_pfn, step) - pbd->min_pfn;
    end_inx = pbd->max_pfn - pbd->min_pfn;

    // 优先从上次分配结束的地方开始分配
    if(pbd->last_hint_inx > bgn_inx)
    {
        fallback = bgn_inx + 1;
        bgn_inx = align_bootmem_index(pbd->last_hint_inx, step);
    }

    while(1)
    {
        int merge;
        void *region;
        unsigned long i, search_end_inx;
        unsigned long start_off, end_off;

find_block:

        bgn_inx = find_next_zero_bit(pbd->bitmap, end_inx - bgn_inx, bgn_inx);
        bgn_inx = align_bootmem_index(bgn_inx, step);

        search_end_inx = bgn_inx + PFN_UP(size);

        if(bgn_inx >= end_inx || search_end_inx > end_inx)
            break;

        for(i=bgn_inx; i<search_end_inx; ++i)
        {
            if(constant_test_bit(i, pbd->bitmap) != 0) {    // space not enough
                bgn_inx = align_bootmem_index(i, step);
                if(bgn_inx == i)
                    bgn_inx += step;

                goto find_block;
            }
        }

        // 如果上次分配截止地址不是页对齐,且是本页的上一页.就尝试利用这段空间.
        if(pbd->last_offset & (PAGE_SIZE - 1) && PFN_DW(pbd->last_offset) + 1 == bgn_inx)
            start_off = align_bootmem_offset(pbd->last_offset, align);
        else
            start_off = pfn2pa(bgn_inx);

        merge = PFN_DW(start_off) < bgn_inx;
        end_off = start_off + size;

        pbd->last_offset = end_off;
        pbd->last_hint_inx = PFN_UP(end_off);

        reserve_bootmem(PFN_DW(start_off) + merge, PFN_UP(end_off));
        
        region = pa2va(pfn2pa(pbd->min_pfn) + start_off);

        memset(region, 0, size);

        return region;
    }

    if(fallback)
    {
        bgn_inx = align_bootmem_index(fallback-1, step);
        fallback = 0;
        goto find_block;
    }

    return 0;
}

void init_mm()
{


}






FreeArea freeArea[MAX_ORDER];
#if 0
unsigned long mb_mm_lower, mb_mm_upper;
unsigned long mb_mmap_addr, mb_mmap_size;
unsigned long mmStart, mmEnd;
unsigned long totalPages;
FreeArea freeArea[MAX_ORDER];

void printBitMap(FreeArea fa)
{
	int i;
	printk("# ");
	for(i=0; i<fa.mapSize; i++)
		printk("%x ", fa.map[i]);
	printk("++++ %d ++++", freeArea[i].count);
	printk(" #\n");
}
void setup_mm()
{
	u32 mm_size = system.mm_size;
	if(mm_size > 1UL<<30)
		mm_size = 1UL<<30;

	printk("mm_size: %x\n", mm_size);

/*
	pmmapItem mmap;
	unsigned long maxAddr;

	mmStart = (va2pa(&end) + (PAGE_SIZE-1)) & PAGE_MASK;
	//mmEnd = (mb_mm_upper) & PAGE_MASK;

	mmap = (pmmapItem)mb_mmap_addr;
	maxAddr = 0;
	int n = 1;
	printk("Boot Loader Provided Physical RAM Map:\n");
	while((unsigned long)mmap < (mb_mmap_addr + mb_mmap_size))
	{
		printk("[%02d] 0x%08x%08x - 0x%08x%08x ",
		n++,//mmap->size,

		mmap->base_addr_high,
		mmap->base_addr_low,
		mmap->length_high,
		mmap->length_low);

		switch(mmap->type)
		{
		case E820_RAM:
			printk("RAM");
			if(maxAddr<(mmap->base_addr_low+mmap->length_low))
			{
				maxAddr = 
				mmap->base_addr_low + mmap->length_low;
			}
			break;
		case E820_RESERVED:
			printk("Reserved");
			break;
		case E820_ACPI:
			printk("ACPI Data");
			break;
		case E820_NVS:
			printk("ACPI NVS");
			break;
		default:
			printk("Unknown %x\n", mmap->type);
			break;
		}
		printk("\n");

		mmap = (pmmapItem) ((unsigned long) mmap
			+ mmap->size + sizeof(mmap->size));
	}

	
	//if(maxAddr < mmEnd)	
	mmEnd = maxAddr & PAGE_MASK;
	if(mmEnd > MAX_SUPT_PHY_MEM_SIZE)
		mmEnd = MAX_SUPT_PHY_MEM_SIZE;
	totalPages = mmEnd >> PAGE_SHIFT;


	// bit map
	int i;
	static char *bitmap_start,*bitmap_end;
	bitmap_start = (char *)mmStart;



	// 
	unsigned long bitmap_size = (mmEnd + 7) / 8;
	bitmap_size =  (bitmap_size + (sizeof(long) - 1UL))
		    &~ (sizeof(long) - 1UL);
	//while(1);
	//memset(bitmap_start, 0xFF, bitmap_size);

#if 0
	printk(	"bitmap_start: %x "
		"mmStart: %x "
		"mmEnd: %x "
		"bitmap_size: %x\n",
		bitmap_start,
		mmStart,
		mmEnd,
		bitmap_size);
#endif
*/
}
#endif
