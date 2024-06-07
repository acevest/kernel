/*
 * ------------------------------------------------------------------------
 *   File Name: dentry.c
 *      Author: Zhao Yanbai
 *              2024-05-15 20:32:49 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <errno.h>
#include <mm.h>
#include <semaphore.h>
#include <string.h>
#include <system.h>
#include <vfs.h>
#define DENTRY_HASH_TABLE_SIZE 233

typedef struct {
    list_head_t list;
    mutex_t mutex;
} dentry_hash_entry_t;

dentry_hash_entry_t dentry_hash_table[DENTRY_HASH_TABLE_SIZE] = {
    0,
};

uint32_t mod64(uint64_t x, uint32_t y) {
    uint32_t mod;

    asm("div %3;" : "=d"(mod) : "a"((uint32_t)x), "d"((uint32_t)(x >> 32)), "r"(y) : "cc");

    return mod;
}

dentry_t *dentry_alloc(dentry_t *parent, qstr_t *s) {
    dentry_t *dentry = NULL;
    atomic_set(&dentry->d_count, 1);
    panic("to do");
    return dentry;
}

dentry_t *dentry_cached_lookup(dentry_t *parent, qstr_t *s) {
    int index = mod64(s->hash, DENTRY_HASH_TABLE_SIZE);
    assert(index < DENTRY_HASH_TABLE_SIZE);

    dentry_hash_entry_t *dhe = dentry_hash_table + index;

    dentry_t *dentry = NULL;

    mutex_lock(&dhe->mutex);

    list_head_t *p;
    list_for_each(p, &dhe->list) {
        dentry = list_entry(p, dentry_t, d_hash);
        assert(dentry != NULL);

        if (dentry->d_name.hash != s->hash) {
            continue;
        }

        if (dentry->d_name.len != s->len) {
            continue;
        }

        if (dentry->d_parent != parent) {
            continue;
        }

        if (memcmp(dentry->d_name.name, s->name, s->len) != 0) {
            continue;
        }

        dentry_get_locked(dentry);

        break;
    }

    mutex_unlock(&dhe->mutex);

    return dentry;
}
int dentry_real_lookup(dentry_t *parent, qstr_t *s, dentry_t **dentry) {
    *dentry = NULL;
    int ret = 0;

    down(&parent->d_inode->i_sem);

    // 在获得信号量后，需要再上cache中查找一遍
    // 因为这个过程中当前进程可能会睡眠，当被唤醒后，其它进程已经在内存准备好了
    *dentry = dentry_cached_lookup(parent, s);

    if (NULL != *dentry) {
        up(&parent->d_inode->i_sem);
        return ret;
    }

    dentry_t *new_dentry = dentry_alloc(parent, s);
    if (new_dentry == NULL) {
        ret = -ENOMEM;
    } else {
        *dentry = parent->d_inode->i_ops->lookup(parent->d_inode, new_dentry);

        // 如果找到了，刚分配的，就不用了
        if (*dentry != NULL) {
            dentry_put(new_dentry);
        }
    }

    up(&parent->d_inode->i_sem);

    return ret;
}

kmem_cache_t *dentry_kmem_cache = NULL;

void dentry_cache_init() {
    kmem_cache_t *dentry_kmem_cache = kmem_cache_create("dentry_cache", sizeof(dentry_t), 4);
    if (NULL == dentry_kmem_cache) {
        panic("create dentry cache faild");
    }

    for (int i = 0; i < DENTRY_HASH_TABLE_SIZE; i++) {
        dentry_hash_entry_t *dhe = dentry_hash_table + i;
        list_init(&dhe->list);
        mutex_init(&dhe->mutex);
    }
}

dentry_t *dentry_get(dentry_t *dentry) {
    assert(dentry != NULL);
    atomic_inc(&dentry->d_count);
    return dentry;
}

void dentry_get_locked(dentry_t *dentry) {}

void dentry_put(dentry_t *dentry) { panic("todo"); }

// static __inline__ struct dentry * dget(struct dentry *dentry)
// {
// 	if (dentry) {
// 		if (!atomic_read(&dentry->d_count))
// 			BUG();
// 		atomic_inc(&dentry->d_count);
// 	}
// 	return dentry;
// }

// static inline struct dentry * __dget_locked(struct dentry *dentry)
// {
// 	atomic_inc(&dentry->d_count);
// 	if (atomic_read(&dentry->d_count) == 1) {
// 		dentry_stat.nr_unused--;
// 		list_del(&dentry->d_lru);
// 		INIT_LIST_HEAD(&dentry->d_lru);		/* make "list_empty()" work */
// 	}
// 	return dentry;
// }

// struct dentry * dget_locked(struct dentry *dentry)
// {
// 	return __dget_locked(dentry);
// }
