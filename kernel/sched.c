/*
 *--------------------------------------------------------------------------
 *   File Name: sched.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Tue Feb 10 11:53:21 2009
 * Last Update: Tue Feb 10 11:53:21 2009
 *
 *--------------------------------------------------------------------------
 */

#include "sched.h"

#include <wait.h>

#include "assert.h"
#include "linkage.h"
#include "mm.h"
#include "msr.h"

task_union root_task __attribute__((__aligned__(PAGE_SIZE)));

// 暂时不考虑pid回绕问题
pid_t get_next_pid() {
    static pid_t g_pid = ROOT_TSK_PID;

    unsigned long iflags;
    irq_save(iflags);

    pid_t pid = g_pid;
    g_pid++;

    irq_restore(iflags);

    return pid;
}

void load_cr3(task_union *tsk) { LoadCR3(tsk->cr3); }

extern pde_t __initdata init_pgd[PDECNT_PER_PAGE] __attribute__((__aligned__(PAGE_SIZE)));

// list_head_t all_tasks;
// list_head_t delay_tasks;
LIST_HEAD(all_tasks);

LIST_HEAD(delay_tasks);

void init_root_task() {
    int i;

    root_task.pid = get_next_pid();
    root_task.ppid = 0;
    root_task.state = TASK_READY;
    root_task.weight = TASK_INIT_WEIGHT;
    root_task.priority = 100;
    strcpy(root_task.name, "root");

    list_add(&root_task.list, &all_tasks);
    // INIT_LIST_HEAD(&root_task.next);

    //  TODO
    // for(i=0; i<NR_OPENS; i++)
    //    root_task.fps[i] = 0;

    root_task.esp0 = ((unsigned long)&root_task) + sizeof(root_task);
    root_task.cr3 = va2pa((unsigned long)(init_pgd));

    tss.esp0 = root_task.esp0;
#if FIX_SYSENTER_ESP_MODE
    // do nothing
#else
    wrmsr(MSR_SYSENTER_ESP, root_task.esp0, 0);
#endif

    printk("init_root_task tss.esp0 %08x\n", tss.esp0);
}

kmem_cache_t *task_union_cache;

void setup_tasks() {
    INIT_LIST_HEAD(&all_tasks);
    INIT_LIST_HEAD(&delay_tasks);

    init_root_task();

    task_union_cache = kmem_cache_create("task_union", sizeof(task_union), PAGE_SIZE);
    if (0 == task_union_cache) {
        panic("setup tasks failed. out of memory");
    }
}

task_union *alloc_task_union() {
    task_union *task;
    task = (task_union *)kmem_cache_alloc(task_union_cache, 0);
    return task;
}

void switch_to() {
    LoadCR3(current->cr3);
    tss.esp0 = current->esp0;
#if FIX_SYSENTER_ESP_MODE
    // do nothing
#else
    wrmsr(MSR_SYSENTER_ESP, current->esp0, 0);
#endif
}

void context_switch(task_union *prev, task_union *next) {
    unsigned long eax, ebx, ecx, edx, esi, edi;
    asm volatile(
        "pushfl;"
        "pushl  %%ebp;"
        "movl   %%esp,%[prev_esp];"
        "movl   %[next_esp],%%esp;"
        "movl   $1f,%[prev_eip];"
        "pushl  %[next_eip];"
        "jmp    switch_to;"
        "1:"
        "popl   %%ebp;"
        "popfl;"
        : [prev_esp] "=m"(prev->esp), [prev_eip] "=m"(prev->eip), "=a"(prev), "=b"(ebx), "=c"(ecx), "=d"(edx),
          "=S"(esi), "=D"(edi)
        : [next_esp] "m"(next->esp), [next_eip] "m"(next->eip), [prev] "a"(prev), [next] "d"(next)
        : "memory");
}

task_union *find_task(pid_t pid) {
    task_union *p = 0;
    list_head_t *pos = 0, *tmp = 0;

    unsigned long iflags;
    irq_save(iflags);
    list_for_each_safe(pos, tmp, &all_tasks) {
        p = list_entry(pos, task_union, list);
        if (p->pid == pid) {
            break;
        }
    }
    irq_restore(iflags);

    return p;
}

static const char *task_state(unsigned int state) {
    static const char s[][16] = {
        "  ERROR", "READY", " WAIT", " INIT", " EXIT",
    };

    if (state >= TASK_END) {
        state = TASK_UNUSED;
    }

    return s[state];
}

unsigned long schedule() {
    task_union *sel = &root_task;
    task_union *p = 0;
    list_head_t *pos = 0, *t = 0;

    unsigned long iflags;
    irq_save(iflags);
    // printl(MPL_ROOT, "root:%d [%08x] cnt %u", root_task.pid, &root_task, root_task.cnt);

#if 1
    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);
        if (p->state != TASK_READY) {
            continue;
        }

        if (p->weight >= p->priority) {
            p->weight = 0;
            continue;
        }

        if (p->weight < sel->weight) {
            sel = p;
        }
    }
#else
    float min_ratio = 1.0;
    bool need_reset_weight = true;
    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);
        if (p->state != TASK_READY) {
            continue;
        }
        if (p->weight < p->priority) {
            need_reset_weight = false;
            break;
        }
    }

    if (need_reset_weight) {
        list_for_each_safe(pos, t, &all_tasks) {
            p = list_entry(pos, task_union, list);
            if (p->state != TASK_READY) {
                continue;
            }
            p->weight = 0;
        }
    }

    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);

        if (p->state != TASK_READY) {
            continue;
        }

        // 貌似在Mac的M1上的qemu执行这一句会有问题
        // 所以暂时把这整个逻辑注释掉，写了个简单的替代算法
        float ratio = (float)(p->weight * 1.0) / (p->priority * 1.0);
        if (ratio < min_ratio) {
            sel = p;
            min_ratio = ratio;
        }
    }
#endif

    irq_restore(iflags);
    sel->sched_cnt++;
    sel->weight += 13;
    // printk("%08x %s weight %d state: %s\n", sel, sel->name, sel->weight, task_state(sel->state));
    task_union *prev = current;
    task_union *next = sel;

    if (prev != next) {
        // printk("switch to: %s:%d\n", next->name, next->pid);
        list_for_each_safe(pos, t, &all_tasks) {
            p = list_entry(pos, task_union, list);
            printl(MPL_TASK_0 + p->pid, " ");  // 清掉上一次显示的 '>'
            printl(MPL_TASK_0 + p->pid, "%s%4s:%d [%08x] state %s weight %03d sched %u", next == p ? ">" : " ", p->name,
                   p->pid, p, task_state(p->state), p->weight, p->sched_cnt);
        }
        context_switch(prev, next);
    }
}

void debug_sched() {
    task_union *p = list_entry(current->list.next, task_union, list);
    p->state = (p->state == TASK_READY) ? TASK_WAIT : TASK_READY;
}
