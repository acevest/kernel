/*
 * ------------------------------------------------------------------------
 *   File Name: file.c
 *      Author: Zhao Yanbai
 *              2024-09-18 12:14:26 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <fs.h>
#include <irq.h>
#include <task.h>

file_t *get_file(int fd) {
    assert(fd >= 0);
    assert(fd < NR_TASK_OPEN_CNT);

    file_t *fp = current->files.fds[fd];

    // TOOD 添加对fp的引用记数

    return fp;
}

//////////
const uint32_t PAGE_HASH_BITS = 10;
const uint32_t PAGE_HASH_SIZE = 1 << PAGE_HASH_BITS;
page_t **page_hash_table = NULL;

static uint32_t page_hash_func(address_space_t *mapping, uint32_t index) {
    uint32_t i = (((uint32_t)mapping) / (sizeof(inode_t) & ~(sizeof(inode_t) - 1)));
#define s(x) ((x) + (x) >> PAGE_HASH_BITS)
    return s(i + index) & (PAGE_HASH_SIZE - 1);
#undef s
}

page_t *page_hash(address_space_t *mapping, uint32_t index) {
    uint32_t hash = page_hash_func(mapping, index);
    assert(hash < PAGE_HASH_SIZE);

    return page_hash_table[hash];
}

page_t *find_hash_page(address_space_t *mapping, uint32_t index) {
    page_t *page = NULL;

    uint32_t hash = page_hash_func(mapping, index);
    assert(hash < PAGE_HASH_SIZE);

    ENTER_CRITICAL_ZONE(EFLAGS);

    page_t *p = page_hash_table[hash];

    while (p != NULL) {
        if (p->mapping != mapping) {
            continue;
        }
        if (p->index == index) {
            page = p;
            break;
        }
    }

    EXIT_CRITICAL_ZONE(EFLAGS);

    return page;
}

void add_page_to_hash(page_t *page, address_space_t *mapping, uint32_t index) {
    uint32_t hash = page_hash_func(mapping, index);
    assert(hash < PAGE_HASH_SIZE);

    ENTER_CRITICAL_ZONE(EFLAGS);
    page_t *p = 0;
    p = page_hash_table[hash];
    page->hash_next = p;
    page_hash_table[hash] = page;
    EXIT_CRITICAL_ZONE(EFLAGS);
}

page_t *get_cached_page(address_space_t *mapping, uint32_t index) {
    page_t *page = NULL;
    page = find_hash_page(mapping, index);

    if (NULL == page) {
        page = alloc_one_page(0);
        assert(page != NULL);

        page->index = index;

        add_page_to_hash(page, mapping, index);

        ENTER_CRITICAL_ZONE(EFLAGS);
        list_add(&page->list, &mapping->pages);
        EXIT_CRITICAL_ZONE(EFLAGS);
    }

    return page;
}

void vfs_page_cache_init() {
    page_hash_table = (page_t **)page2va(alloc_one_page(0));
    assert(page_hash_table != NULL);
    memset(page_hash_table, 0, PAGE_SIZE);
}
