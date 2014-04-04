/*
 * ------------------------------------------------------------------------
 *   File Name: buddy.c
 *      Author: Zhao Yanbai
 *              Fri Apr  4 19:25:02 2014
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <mm.h>

#define MAX_ORDER       4
struct buddy_system
{
    page_t      **page_map;
    free_area_t free_area[MAX_ORDER + 1];
};

struct buddy_system buddy_system;

void init_buddy_system()
{
    unsigned long pfn_cnt = bootmem_total_pages();

    buddy_system.page_map = alloc_bootmem(pfn_cnt*sizeof(page_t*), sizeof(page_t*));
    if(0 == buddy_system.page_map) {
        printk("can not go on playing...\n");
        while(1);
    }

    unsigned long i;
    for(i=0; i<pfn_cnt; ++i)
    {
        buddy_system.page_map[i] = alloc_bootmem(sizeof(page_t), sizeof(unsigned long));
        if(0 == buddy_system.page_map[i]) {
            printk("can not go on playing...\n");
            while(1);
        }
    }

    for(i=0; i<pfn_cnt; ++i)
    {
        if(BOOTMEM_PAGE_FREE == bootmem_page_state(i))
        {
            // free to buddy system
        }
        
        buddy_system.page_map[i]->index = i;
    }
}
