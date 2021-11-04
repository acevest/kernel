/*
 * ------------------------------------------------------------------------
 *   File Name: console.c
 *      Author: Zhao Yanbai
 *              Sun Jun 22 18:50:13 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <console.h>
#include <sched.h>
#include <string.h>
#include <wait.h>

void vga_putc(unsigned int nr, unsigned char c, const unsigned char color);

cnsl_t cnsl;

static bool empty(const cnsl_queue_t *q) { return q->head == q->tail; }

static bool full(const cnsl_queue_t *q) { return (q->head + 1) % CNSL_QUEUE_SIZE == q->tail; }

static void put(cnsl_queue_t *q, char c) {
    if (!full(q)) {
        q->data[q->head] = c;
        q->head = (q->head + 1) % CNSL_QUEUE_SIZE;
    }
}

static bool get(cnsl_queue_t *q, char *c) {
    if (!empty(q)) {
        *c = q->data[q->tail];
        q->tail = (q->tail + 1) % CNSL_QUEUE_SIZE;
        return true;
    }

    return false;
}

static void clear(cnsl_queue_t *q) { q->head = q->tail = 0; }

static void erase(cnsl_queue_t *q) {
    if (empty(q)) return;

    if (q->head == 0)
        q->head = CNSL_QUEUE_SIZE - 1;
    else
        q->head--;
}

static void cnsl_queue_init(cnsl_queue_t *cq) {
    memset((void *)cq, 0, sizeof(*cq));
    cq->head = 0;
    cq->tail = 0;
    init_wait_queue(&cq->wait);
}

wait_queue_head_t rdwq;

void cnsl_init() {
    cnsl_queue_init(&cnsl.rd_q);
    cnsl_queue_init(&cnsl.wr_q);
    cnsl_queue_init(&cnsl.sc_q);
    init_wait_queue(&rdwq);
}

int cnsl_kbd_write(char ch) {
    if (ch == 0) return 0;

    if (ch == '\b') {
        if (!empty(&cnsl.wr_q)) vga_putc(0, '\b', 0xF);
        erase(&cnsl.wr_q);
        erase(&cnsl.sc_q);
    } else {
        put(&cnsl.wr_q, ch);
        put(&cnsl.sc_q, ch);
        vga_putc(0, ch, 0xF);
    }

    if (ch == '\n') {
        clear(&cnsl.wr_q);
        while (get(&cnsl.sc_q, &ch)) put(&cnsl.rd_q, ch);
        wake_up(&rdwq);
    }
}

int cnsl_read(char *buf, size_t count) {
    unsigned long flags;

    assert(count > 0);
    int cnt = 0;
    for (cnt = 0; cnt < count;) {
        char ch;
        task_union *task = current;
        DECLARE_WAIT_QUEUE(wait, task);
        add_wait_queue(&rdwq, &wait);

        while (true) {
            task->state = TASK_WAIT;
            irq_save(flags);
            bool r = get(&cnsl.rd_q, &ch);
            irq_restore(flags);

            if (r) {
                buf[cnt++] = ch;

                task->state = TASK_RUNNING;
                del_wait_queue(&rdwq, &wait);

                if (ch == '\n') goto end;

                break;
            }

            schedule();
        }
    }

end:
    buf[cnt] = 0;
    return cnt;
}

chrdev_t cnsl_chrdev = {.read = cnsl_read};
