#include <stdio.h>
#include <stdlib.h>

/* Allmost Copy From Linux */
typedef struct list_head {
    struct list_head *prev, *next;
} list_head_t;

#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) list_head_t name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr)  \
    do {                     \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)

#define list_entry(ptr, type, member) ((type*)((char*)(ptr) - (unsigned long)(&((type*)0)->member)))

#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

#define list_for_each_entry_safe(pos, tmp, head, member)          \
    for (pos = list_entry((head)->next, typeof(*pos), member),    \
        tmp = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); pos = tmp, tmp = list_entry(tmp->member.next, typeof(*tmp), member))

static inline void _list_add(list_head_t* pnew, list_head_t* prev, list_head_t* next) {
    next->prev = pnew;
    pnew->next = next;
    pnew->prev = prev;
    prev->next = pnew;
}

static inline void list_add(list_head_t* pnew, list_head_t* head) {
    _list_add(pnew, head, head->next);
}

static inline void list_add_tail(list_head_t* pnew, list_head_t* head) {
    _list_add(pnew, head->prev, head);
}

static inline void _list_del(list_head_t* prev, list_head_t* next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(list_head_t* entry) {
    _list_del(entry->prev, entry->next);
    entry->prev = NULL;
    entry->next = NULL;
}

static inline void list_del_init(list_head_t* entry) {
    _list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

static inline int list_empty(list_head_t* head) {
    return head->next == head;
}

typedef struct node {
    int id;
    list_head_t list;
    list_head_t pend;
} node_t;

LIST_HEAD(allH);
LIST_HEAD(pendH);

int main() {
    INIT_LIST_HEAD(&allH);
    INIT_LIST_HEAD(&pendH);

    for (int i = 0; i < 10; i++) {
        node_t* n = (node_t*)malloc(sizeof(node_t));
        n->id = i;
        list_add(&n->list, &allH);
        if (n->id % 3 == 0) {
            list_add(&n->pend, &pendH);
        }
    }

    list_head_t* pos;
    list_head_t* tmp;
    node_t *p, *p1;
    list_for_each_safe(pos, tmp, &allH) {
        p = list_entry(pos, node_t, list);
        printf("allH: %d\n", p->id);
    }

    printf("-----\n");
    list_for_each_safe(pos, tmp, &pendH) {
        p = list_entry(pos, node_t, pend);
        printf("pendH: %d\n", p->id);
    }

    // list_for_each_safe(pos, tmp, &allH) {

    //    p = list_entry(pos, node_t, list);
    list_for_each_entry_safe(p, p1, &pendH, pend) {
        // printf("%d\n", p->id);
        if (p->id == 3) {
            list_del(&p->pend);
        }
    }

    printf("-----\n");

    list_for_each_safe(pos, tmp, &allH) {
        p = list_entry(pos, node_t, list);
        printf("allH: %d\n", p->id);
    }

    printf("-----\n");
    list_for_each_safe(pos, tmp, &pendH) {
        p = list_entry(pos, node_t, pend);
        printf("pendH: %d\n", p->id);
    }
}
