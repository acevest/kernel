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
    root_task.priority = 7;
    root_task.ticks = root_task.priority;
    root_task.turn = 0;
    root_task.sched_cnt = 0;
    root_task.sched_keep_cnt = 0;
    root_task.magic = TASK_MAGIC;
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
        "  ERROR", "RUNNING", "  READY", "   WAIT", "   INIT", "   EXIT",
    };

    if (state >= TASK_END) {
        state = TASK_UNUSED;
    }

    return s[state];
}

extern uint32_t disk_request_cnt;
extern uint32_t disk_handled_cnt;
extern uint32_t disk_inter_cnt;

void debug_print_all_tasks() {
    task_union *p = 0;
    list_head_t *pos = 0, *t = 0;
    printl(MPL_TASK_TITLE, "         NAME     STATE TK/PI TURN       SCHED      KEEP");
    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);
        printl(MPL_TASK_0 + p->pid, "%08x%s%4s:%u %s %02u/%02u %-10u %-10u %-10u", p,
               p->state == TASK_RUNNING ? ">" : " ", p->name, p->pid, task_state(p->state), p->ticks, p->priority,
               p->turn, p->sched_cnt, p->sched_keep_cnt);
    }
}

void schedule() {
    task_union *root = &root_task;
    task_union *sel = 0;
    task_union *p = 0;
    list_head_t *pos = 0, *t = 0;
    printk("*");

    printl(MPL_X, "disk req %u consumed %u irq %u", disk_request_cnt, disk_handled_cnt, disk_inter_cnt);

    assert(current->ticks <= TASK_MAX_PRIORITY);
    assert(current->priority <= TASK_MAX_PRIORITY);

    unsigned long iflags;
    irq_save(iflags);

    if (TASK_RUNNING == current->state) {
        if (0 == current->ticks) {
            current->turn++;
            current->ticks = current->priority;
            current->state = TASK_READY;
        } else {
            irq_restore(iflags);
            return;
        }
    }

    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);

        assert(p->state != TASK_RUNNING);

        if (p == &root_task) {
            continue;
        }

        if (TASK_READY != p->state) {
            continue;
        }

        if (sel == 0) {
            sel = p;
            continue;
        }
#if 1
        // 考察三个量
        // priority 越大越优先
        // jiffies  越小越优先
        // (priority - ticks) 表示已经使用的量，越小越优先
        int64_t a = sel->jiffies - sel->priority + (sel->priority - sel->ticks);
        int64_t b = p->jiffies - p->priority + (p->priority - p->ticks);
        if (a > b) {
            sel = p;
        } else if (a == b) {
            if (sel->priority < p->priority) {
                sel = p;
            }
        }
#else
        if (sel->jiffies < p->jiffies) {
            continue;
        }

        uint64_t delta = sel->jiffies - p->jiffies;

        if (sel->priority <= p->priority) {
            if (delta > (1 * p->ticks)) {
                sel = p;
            }
        } else if (sel->priority > p->priority) {
            if (delta > (5 * p->ticks)) {
                sel = p;
            }
        }
#endif
    }

    task_union *prev = current;
    task_union *next = sel != 0 ? sel : root;

    next->state = TASK_RUNNING;

#if 1
    // debug_print_all_tasks();
#else
    printl(MPL_TASK_TITLE, "         NAME     STATE TK/PI TURN       SCHED      KEEP");
    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);
        printl(MPL_TASK_0 + p->pid, "%08x%s%4s:%d %s %02u/%02d %-10u %-10u %-10u", p, next == p ? ">" : " ", p->name,
               p->pid, task_state(p->state), p->ticks, p->priority, p->turn, p->sched_cnt, p->sched_keep_cnt);
    }
#endif
    if (prev != next) {
        next->sched_cnt++;
        context_switch(prev, next);
    } else {
        // 这里可能是的情况是任务把时间片ticks用完了
        // 被设置成READY
        // 重新高度，还是选中了该任务
        next->sched_keep_cnt++;
    }

    assert(current->state == TASK_RUNNING);
    irq_restore(iflags);
}

void debug_sched() {
    task_union *p = list_entry(current->list.next, task_union, list);
    p->state = (p->state == TASK_READY) ? TASK_WAIT : TASK_READY;
}
