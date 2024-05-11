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

task_t root_task __attribute__((__aligned__(PAGE_SIZE)));

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

void load_cr3(task_t *tsk) { LoadCR3(tsk->cr3); }

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
    root_task.reason = "root";
    root_task.priority = 7;
    root_task.ticks = root_task.priority;
    root_task.turn = 0;
    root_task.need_resched = 0;
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
#if FIXED_SYSENTER_ESP_MODE
    // do nothing
#else
    wrmsr(MSR_SYSENTER_ESP, root_task.esp0, 0);
#endif

    for (i = 0; i < NR_TASK_OPEN_CNT; i++) {
        root_task.files.fds[i] = NULL;
    }

    printk("init_root_task tss.esp0 %08x\n", tss.esp0);
}

kmem_cache_t *task_t_cache;

void setup_tasks() {
    INIT_LIST_HEAD(&all_tasks);
    INIT_LIST_HEAD(&delay_tasks);

    init_root_task();

    task_t_cache = kmem_cache_create("task_t", sizeof(task_t), PAGE_SIZE);
    if (0 == task_t_cache) {
        panic("setup tasks failed. out of memory");
    }
}

task_t *alloc_task_t() {
    task_t *task;
    task = (task_t *)kmem_cache_alloc(task_t_cache, 0);
    return task;
}

void switch_to() {
    LoadCR3(current->cr3);
    tss.esp0 = current->esp0;
#if FIXED_SYSENTER_ESP_MODE
    // do nothing
#else
    wrmsr(MSR_SYSENTER_ESP, current->esp0, 0);
#endif
}

void context_switch(task_t *prev, task_t *next) {
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

task_t *find_task(pid_t pid) {
    task_t *p = 0;
    list_head_t *pos = 0, *tmp = 0;

    unsigned long iflags;
    irq_save(iflags);
    list_for_each_safe(pos, tmp, &all_tasks) {
        p = list_entry(pos, task_t, list);
        if (p->pid == pid) {
            break;
        }
    }
    irq_restore(iflags);

    return p;
}

const char *task_state(unsigned int state) {
    static const char s[][8] = {
        " ERROR", "\x10RUN\x07\x07", " READY", " WAIT ", " INIT ", " EXIT ",
    };

    if (state >= TASK_END) {
        state = TASK_UNUSED;
    }

    return s[state];
}

void debug_print_all_tasks() {
    task_t *p = 0;
    list_head_t *pos = 0, *t = 0;
    printl(MPL_TASK_TITLE, "         NAME      STATE TK/PI REASON     SCHED      KEEP       TURN");
    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_t, list);
        printl(MPL_TASK_0 + p->pid, "%08x %-6s:%u %s %02u/%02u %-10s %-10u %-10u %-10u", p, p->name, p->pid,
               task_state(p->state), p->ticks, p->priority, p->reason, p->sched_cnt, p->sched_keep_cnt, p->turn);
    }
}

void schedule() {
    task_t *root = &root_task;
    task_t *sel = 0;
    task_t *p = 0;
    list_head_t *pos = 0, *t = 0;

    assert(current->ticks >= 0);
    assert(current->priority <= TASK_MAX_PRIORITY);

    unsigned long iflags;
    irq_save(iflags);

    if (current->state == TASK_RUN) {
        current->state = TASK_READY;
    }

    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_t, list);

        if (p == &root_task) {
            continue;
        }

        assert(p->state != TASK_RUN);

        if (TASK_READY != p->state) {
            continue;
        }

        if (sel == 0) {
            sel = p;
            continue;
        }

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
    }

    task_t *prev = current;
    task_t *next = sel != 0 ? sel : root;

    prev->need_resched = 0;

    next->state = TASK_RUN;
    next->reason = "";

    if (prev != next) {
        next->sched_cnt++;
        context_switch(prev, next);
    } else {
        // 这里可能是的情况是任务把时间片ticks用完了
        // 被设置成READY
        // 重新高度，还是选中了该任务
        next->sched_keep_cnt++;
    }

    assert(current->state == TASK_RUN);

    irq_restore(iflags);
}

void debug_sched() {
    task_t *p = list_entry(current->list.next, task_t, list);
    p->state = (p->state == TASK_READY) ? TASK_WAIT : TASK_READY;
}
