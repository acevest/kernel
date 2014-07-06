/*
 *--------------------------------------------------------------------------
 *   File Name: list.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Mon Apr 20 20:52:05 2009
 * Last Update: Mon Apr 20 20:52:05 2009
 * 
 *--------------------------------------------------------------------------
 */

#pragma once

/* Allmost Copy From Linux */
typedef struct list_head
{
    struct list_head *prev, *next;
} list_head_t;

// TODO Remove
typedef list_head_t ListHead, *pListHead;

#define LIST_HEAD_INIT(name) {&(name), &(name) }
#define LIST_HEAD(name) list_head_t name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr)     \
do{                             \
    (ptr)->next = (ptr);        \
    (ptr)->prev = (ptr);        \
}while(0)

#define list_entry(ptr, type, member)       \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)


#define list_for_each(pos, head)            \
    for(pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, tmp, head)  \
    for(pos = (head)->next, tmp = pos->next;\
        pos != (head);                      \
        pos = tmp, tmp = pos->next)

#define list_for_each_entry_safe(pos, tmp, head, member)        \
    for(pos=list_entry((head)->next, typeof(*pos), member),     \
        tmp=list_entry(pos->member.next, typeof(*pos), member); \
        &pos->member != (head);                                 \
        pos=tmp, tmp=list_entry(tmp->member.next, typeof(*tmp), member))


static inline void _list_add(list_head_t *pnew, list_head_t *prev, list_head_t *next)
{
    next->prev    = pnew;
    pnew->next    = next;
    pnew->prev    = prev;
    prev->next    = pnew;
}

static inline void list_add(list_head_t *pnew, list_head_t *head)
{
    _list_add(pnew, head, head->next);
}

static inline void list_add_tail(list_head_t *pnew, list_head_t *head)
{
    _list_add(pnew, head->prev, head);
}


static inline void _list_del(list_head_t *prev, list_head_t *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(list_head_t *entry)
{
    _list_del(entry->prev, entry->next);
}

static inline void list_del_init(list_head_t *entry)
{
    _list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

static inline int list_empty(list_head_t *head)
{
    return head->next == head;
}
