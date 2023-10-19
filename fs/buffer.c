/*
 * ------------------------------------------------------------------------
 *   File Name: buffer.c
 *      Author: Zhao Yanbai
 *              2023-06-20 19:30:33 Tuesday CST
 * Description: 目前只支持只读，因此可以先写得简单一点
 * ------------------------------------------------------------------------
 */

#include <buffer.h>

#define MAX_BBUFFER_CNT 985

#define BLOCK_BUFFER_HASH_TABLE_SIZE 211
// atomic_t hash_cnt;
list_head_t block_buffer_hash_table[BLOCK_BUFFER_HASH_TABLE_SIZE] = {
    0,
};

typedef struct bbuffer_store {
    int blocksize;
    list_head_t cache_list;
    list_head_t free_list;
} bbuffer_store_t;

// 1024, 2048, 4096
bbuffer_store_t store[3] = {0};

int hashfn(dev_t dev, uint32_t block) { return ((dev) ^ block) % BLOCK_BUFFER_HASH_TABLE_SIZE; }

bbuffer_t *get_hash_block_buffer(dev_t dev, uint32_t block, uint16_t size) {}

kmem_cache_t *bbufer_kmem_cache = 0;

bbuffer_t *bread(dev_t dev, uint64_t block, uint32_t size) {
    assert(size == 1024 || size == 2048 || size == 4096);

    // 暂时先只支持hard disk
    assert(DEV_MAJOR(dev) == DEV_MAJOR_DISK);

    // 先尝试从hash里分配
    list_head_t *p = 0;
    uint32_t hash = hashfn(dev, block);
    list_for_each(p, block_buffer_hash_table + hash) {
        bbuffer_t *b = list_entry(p, bbuffer_t, node);
        if (b->dev != dev) {
            continue;
        }
        if (b->block == block) {
            continue;
        }

        break;
    }

    // 如果找到了就直接返回
    if (0 != p) {
        bbuffer_t *b = list_entry(p, bbuffer_t, node);
        assert(0 != b);
        assert(0 != b->data);
        return b;
    }

    // 如果没找到，则从store的cached_list里尝试找

    // 如果没找到，则尝试从store里的free_list里尝试分配

    // 如果free_list为空，且cached_list不为空，
}

void init_buffer() {
    for (int i = 0; i < BLOCK_BUFFER_HASH_TABLE_SIZE; i++) {
        list_init(block_buffer_hash_table + i);
    }

    bbufer_kmem_cache = kmem_cache_create("bbuffer", sizeof(bbuffer_t), 4);
    if (0 == bbufer_kmem_cache) {
        panic("no memory for bbufer kmem cache");
    }

    printk("bbufer kmem cache %08x\n", bbufer_kmem_cache);

    for (int i = 0; i < 3; i++) {
        int blocksize = 1 << (10 + i);

        store[i].blocksize = blocksize;
        list_init(&store[i].cache_list);
        list_init(&store[i].free_list);

        int page_left_space = 0;
        void *data = 0;
        page_t *page = 0;
        for (int j = 0; j < MAX_BBUFFER_CNT; j++) {
            if (page_left_space < blocksize) {
                data = (void *)(alloc_one_page(0));
                page = va2page(data);
            }

            bbuffer_t *b = kmem_cache_alloc(bbufer_kmem_cache, 0);
            assert(b != 0);

            b->block = 0;
            b->block_size = blocksize;
            b->ref_count = 0;
            b->data = data + j * blocksize;
            b->dev = 0;
            b->page = page;
            list_init(&b->node);

            assert(0 != b->data);

            list_add(&b->node, &store[i].free_list);

            page_left_space -= blocksize;
        }
    }
}
