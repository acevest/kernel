/*
 * ------------------------------------------------------------------------
 *   File Name: buffer.c
 *      Author: Zhao Yanbai
 *              2023-06-20 19:30:33 Tuesday CST
 * Description: 目前只支持只读，因此可以先写得简单一点
 * ------------------------------------------------------------------------
 */

#include <buffer.h>
#include <irq.h>

#define MAX_BBUFFER_CNT 985

#define BLOCK_BUFFER_HASH_TABLE_SIZE 211
// atomic_t hash_cnt;
list_head_t block_buffer_hash_table[BLOCK_BUFFER_HASH_TABLE_SIZE] = {
    0,
};

int hashfn(dev_t dev, uint32_t block) {
    return ((dev) ^ block) % BLOCK_BUFFER_HASH_TABLE_SIZE;
}

typedef struct bbuffer_store {
    int blocksize;
    // list_head_t cache_list;
    list_head_t free_list;
    wait_queue_head_t waitq;
} bbuffer_store_t;

// 1024, 2048, 4096
bbuffer_store_t store[3] = {0};

kmem_cache_t* bbufer_kmem_cache = 0;

bbuffer_store_t* getstore(uint32_t size) {
    assert(size == 1024 || size == 2048 || size == 4096);

    bbuffer_store_t* s = 0;

    if (1024 == size) {
        s = store + 0;
    } else if (2048 == size) {
        s = store + 1;
    } else if (4096 == size) {
        s = store + 2;
    }

    assert(0 != s);

    return s;
}

bbuffer_t* get_from_hash_table(dev_t dev, uint64_t block, uint32_t size) {
    list_head_t* p = 0;
    bbuffer_t* b = 0;

    uint32_t hash = hashfn(dev, block);
    assert(hash < BLOCK_BUFFER_HASH_TABLE_SIZE);
    list_for_each(p, block_buffer_hash_table + hash) {
        bbuffer_t* t = list_entry(p, bbuffer_t, node);
        if (t->dev != dev) {
            continue;
        }

        if (t->block != block) {
            continue;
        }

        assert(t->block_size == size);

        b = t;

        break;
    }

    // 如果找到了就直接返回
    if (b != NULL) {
        atomic_inc(&b->ref_count);
        assert(0 != b);
        assert(0 != b->data);

        // 等待buffer的lock释放
        // wait on b
        return b;
    }

    return NULL;
}

bbuffer_t* getblk(dev_t dev, uint64_t block, uint32_t size) {
    uint32_t iflags;
    assert(size == 1024 || size == 2048 || size == 4096);

    // 暂时先只支持hard disk
    assert(DEV_MAJOR(dev) == DEV_MAJOR_DISK);

    int retry = 0;
again:
    irq_save(iflags);

    // 先尝试从hash里分配
    bbuffer_t* b = get_from_hash_table(dev, block, size);
    if (NULL != b) {
        return b;
    }

    // 如果没找到，则从store的free_list里尝试找到第一个ref_count == 0 且未上锁的
    bbuffer_store_t* s = getstore(size);
    list_head_t* p = 0;
    list_for_each(p, &s->free_list) {
        bbuffer_t* t = list_entry(p, bbuffer_t, node);
        assert(NULL != t);
        assert(t->block_size == size);

        // if (t->ref_count == 0) {
        if (atomic_read(&t->ref_count) == 0) {
            b = t;
            break;
        }
    }

    // 如果还是没有找到，则就需要等待释放出新的空间
    if (b == NULL) {
        irq_restore(iflags);
        retry++;
        // wait on free list
        // TODO
        assert(0);
        // wait_on(&s->waitq);
        goto again;
    }

    // 把它从free_list上删掉
    list_del_init(&b->node);

    // FIXME
    irq_restore(iflags);

    // 虽然此时该bbuffer_t上的ref_count为0但其可能还有I/O操作没有完成
    // 因为可能有的进程调用了write、read后再直接调用brelse
    // 所以需要在此等待其结束
    wait_completion(&b->io_done);

    // 找到了
    b->block = block;
    b->block_size = size;
    b->dev = dev;
    atomic_set(&(b->ref_count), 1);
    b->uptodate = 0;

    return b;
}

void brelse(bbuffer_t* b) {
    assert(b != NULL);
    assert(atomic_read(&(b->ref_count)) > 0);

    wait_completion(&b->io_done);

    bbuffer_store_t* s = getstore(b->block_size);
    assert(s != NULL);
    assert(s - store < 3);

    // TODO
    assert(0);
    // wake_up(&s->waitq);
}

bbuffer_t* bread(dev_t dev, uint64_t block, uint32_t size) {
    bbuffer_t* b = getblk(dev, block, size);

    assert(b != NULL);

    if (b->uptodate == 1) {
        return b;
    }

    // READ
    void block_read(bbuffer_t * b);
    block_read(b);

    // 等待I/O结束
    wait_completion(&b->io_done);
    if (b->uptodate == 1) {
        return b;
    }

    brelse(b);

    return NULL;
}

void init_buffer() {
    for (int i = 0; i < BLOCK_BUFFER_HASH_TABLE_SIZE; i++) {
        list_init(block_buffer_hash_table + i);
    }

    bbufer_kmem_cache = kmem_cache_create("bbuffer", sizeof(bbuffer_t), 4);
    if (NULL == bbufer_kmem_cache) {
        panic("no memory for bbufer kmem cache");
    }

    printk("bbufer kmem cache %08x\n", bbufer_kmem_cache);
    printk("SIZE %u\n", sizeof(bbuffer_t));

    for (int i = 0; i < 3; i++) {
        int blocksize = 1 << (10 + i);

        store[i].blocksize = blocksize;
        // list_init(&store[i].cache_list);
        list_init(&store[i].free_list);
        init_wait_queue_head(&store[i].waitq);

        int page_left_space = 0;
        void* data = NULL;
        page_t* page = NULL;
        for (int j = 0; j < MAX_BBUFFER_CNT; j++) {
            if (page_left_space < blocksize) {
                data = (void*)page2va(alloc_one_page(0));
                page = va2page(data);
                page_left_space = PAGE_SIZE;

                // printk("blocksize %u\n", blocksize);
                // printk("page[%u] %08x\n", page->index, data);
            }

            bbuffer_t* b = kmem_cache_alloc(bbufer_kmem_cache, 0);
            assert(NULL != b);

            b->block = 0;
            b->block_size = blocksize;
            atomic_set(&(b->ref_count), 0);
            // b->data = data + (page_left_space - blocksize);
            b->data = data + (PAGE_SIZE - page_left_space);
            b->dev = 0;
            b->page = page;
            b->uptodate = 0;
            init_completion(&b->io_done);
            complete(&b->io_done);
            list_init(&b->node);

            assert(NULL != b->data);

            list_add(&b->node, &store[i].free_list);

            // printk("[%u] bbuffer[%u].data %08x\n", blocksize, j, b->data);

            page_left_space -= blocksize;
        }
    }
}
