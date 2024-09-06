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

static kmem_cache_t *g_dentry_kmem_cache = NULL;
typedef struct {
    list_head_t list;
    mutex_t mutex;
} dentry_hash_entry_t;

dentry_hash_entry_t dentry_hash_table[DENTRY_HASH_TABLE_SIZE] = {
    0,
};

uint32_t mod64(uint64_t x, uint32_t y) {
#if 1
    // TODO FIXME
    uint32_t a = (uint32_t)x;
    uint32_t b = (x >> 32);

    a %= y;
    b %= y;

    return (a + b) % y;
#endif
#if 0
    uint32_t mod;

    asm("div %3;" : "=d"(mod) : "a"((uint32_t)x), "d"((uint32_t)(x >> 32)), "r"(y) : "cc");

    return mod;
#endif
}

dentry_t *dentry_alloc(dentry_t *parent, const qstr_t *s) {
    dentry_t *dentry = NULL;

    assert(s != NULL);
    assert(s->len > 0);
    assert(s->len < DENTRY_INLINE_NAME_LEN - 1);

    dentry = kmem_cache_zalloc(g_dentry_kmem_cache, 0);
    if (dentry == NULL) {
        panic("no mem for dentry");
        return dentry;
    }

    dentry->d_flags = 0;

    dentry->d_name.len = 0;
    dentry->d_name.name = 0;
    dentry->d_name = *s;

    memcpy(dentry->d_inline_name, s->name, s->len);
    dentry->d_inline_name[s->len] = 0;

    atomic_set(&dentry->d_count, 1);

    if (parent != NULL) {
        dentry->d_parent = parent;
    } else {
        dentry->d_parent = dentry;
    }

    INIT_LIST_HEAD(&dentry->d_child);
    INIT_LIST_HEAD(&dentry->d_subdirs);
    INIT_LIST_HEAD(&dentry->d_hash);

    dentry->d_sb = NULL;
    dentry->d_inode = NULL;
    dentry->d_ops = NULL;
    dentry->d_private = NULL;

    return dentry;
}

dentry_t *dentry_alloc_root(inode_t *root_inode) {
    dentry_t *dentry;

    assert(root_inode != NULL);

    static const qstr_t name = {.name = "/", .len = 1, .hash = 0};
    dentry = dentry_alloc(NULL, &name);
    if (dentry != NULL) {
        dentry->d_sb = root_inode->i_sb;
        dentry->d_parent = dentry;
    }

    // dentry->d_inode = root_inode;
    dentry_add(dentry, root_inode);

    return dentry;
}

dentry_hash_entry_t *dentry_hash(dentry_t *parent, uint64_t hash) {
    int index = mod64(hash, DENTRY_HASH_TABLE_SIZE);
    assert(index < DENTRY_HASH_TABLE_SIZE);

    dentry_hash_entry_t *dhe = dentry_hash_table + index;

    assert(dhe != NULL);
    assert(dhe >= dentry_hash_table);
    assert(dhe < dentry_hash_table + DENTRY_HASH_TABLE_SIZE);

    return dhe;
}

void dentry_attach_inode(dentry_t *dentry, inode_t *inode) {
    assert(dentry != NULL);
    // assert(inode != NULL);

    dentry->d_inode = inode;
}

void dentry_rehash(dentry_t *dentry) {
    dentry_hash_entry_t *dhe = dentry_hash(dentry->d_parent, dentry->d_name.hash);

    mutex_lock(&dhe->mutex);

    list_add(&dentry->d_hash, &dhe->list);

    mutex_unlock(&dhe->mutex);
}

void dentry_add(dentry_t *dentry, inode_t *inode) {
    dentry_attach_inode(dentry, inode);
    dentry_rehash(dentry);
}

dentry_t *dentry_cached_lookup(dentry_t *parent, qstr_t *s) {
    dentry_hash_entry_t *dhe = dentry_hash(parent, s->hash);

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

        mutex_unlock(&dhe->mutex);
        return dentry;
    }

    mutex_unlock(&dhe->mutex);

    return NULL;
}
int dentry_real_lookup(dentry_t *parent, qstr_t *s, dentry_t **dentry) {
    *dentry = NULL;
    int ret = 0;

    assert(parent->d_inode != NULL);
    inode_t *dir = parent->d_inode;

    down(&dir->i_sem);

    // 在获得信号量后，需要再上cache中查找一遍
    // 因为这个过程中当前进程可能会睡眠，当被唤醒后，其它进程已经在内存准备好了
    *dentry = dentry_cached_lookup(parent, s);

    if (NULL != *dentry) {
        up(&dir->i_sem);
        return ret;
    }

    dentry_t *new_dentry = dentry_alloc(parent, s);
    if (new_dentry == NULL) {
        ret = -ENOMEM;
    } else {
        assert(dir->i_ops != NULL);
        assert(dir->i_ops->lookup != NULL);
        printk(">>>>>> %x %x\n", dir->i_ops, dir->i_ops->lookup);
        *dentry = dir->i_ops->lookup(dir, new_dentry);
        // 返回 lookup 没有再分配一个dentry
        // 否则就释放dentry_new使用lookup返回的dentry
        if (*dentry == NULL) {
            *dentry = new_dentry;
        } else {
            dentry_put(new_dentry);
        }
        // if (ret == 0) {  // 返回0才代表成功
        //     *dentry = new_dentry;
        // } else {
        //     dentry_put(new_dentry);
        // }
    }

    up(&dir->i_sem);

    return ret;
}

void dentry_cache_init() {
    g_dentry_kmem_cache = kmem_cache_create("dentry_cache", sizeof(dentry_t), 4);
    if (NULL == g_dentry_kmem_cache) {
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

void dentry_get_locked(dentry_t *dentry) {
    //
}

void dentry_put(dentry_t *dentry) {
    //
}

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
