/*
 *--------------------------------------------------------------------------
 *   File Name: clock.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Jan  5 09:51:54 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#include <io.h>
#include <printk.h>
#include <sched.h>
#include <system.h>
#include <wait.h>

volatile uint64_t jiffies = 0;  // TODO uint64: undefined reference to `__umoddi3'

unsigned int sys_clock() { return jiffies; }

void debug_print_all_tasks();

void dump_irq_nr_stack();
void clk_bh_handler(void *arg);

void clk_handler(unsigned int irq, pt_regs_t *regs, void *dev_id) {
    // if (jiffies % 100 == 0) {
    // printl(MPL_CLOCK, "clock irq: %d", jiffies);
    printlxy(MPL_IRQ, MPO_CLOCK, "CLK irq: %d", jiffies);
    // }

    jiffies++;

    current->jiffies = jiffies;

    // 中断目前虽然不能嵌套，但依然可以打断前一个中断的下半部分处理
    // 若前一个时钟中断将这个值减到0
    // 同时其下半部分处理时间过长，直到这个时钟中断还没处理完
    // 那么这个时钟中断是完全可以打断它，且在这里把这个ticks从0减到负数
    // 而这个是uint32_t型，因此会溢出成0xFFFFFFFF
    if (current->ticks > 0) {
        current->ticks--;
    }

    assert(current->ticks <= TASK_MAX_PRIORITY);  // 防止ticks被减到0后再减溢出

    add_irq_bh_handler(clk_bh_handler, NULL);
}

// 开中断执行这个函数
// 后续放到一个内核任务中去做，需要先把禁止内核抢占做了
const char *task_state(unsigned int state);
void clk_bh_handler(void *arg) {
    task_t *p = 0;
    list_head_t *t = 0;
    list_head_t *pos = 0;
    list_for_each_safe(pos, t, &delay_tasks) {
        p = list_entry(pos, task_t, pend);
        // printk("%s state: %s\n", p->name, task_state(p->state));
        assert(p->state == TASK_WAIT);
        assert(p->delay_jiffies != 0);
        if (p->delay_jiffies > 0 && jiffies > p->delay_jiffies) {
            list_del(&p->pend);
            p->delay_jiffies = 0;
            p->state = TASK_READY;
            p->reason = "clk_bh";
        }
    }

    // 此处调用这个还是有问题的
    debug_print_all_tasks();
}

void setup_i8253(uint16_t hz) {
    // 最低频率为18.2Hz(1193180/65536，最大计数值是65536是因为往计数器里写0就从65536开始计数)
    assert(hz >= 19);

    const uint8_t counter_no = 0;  // 第0个计数器
    const uint8_t read_write_latch = 3;  // 0 锁存数据供CPU读；1只读写低字节；2只读写高字节；3先读写低字节，后读写高字节
    const uint8_t mode = 2;  //

    const uint8_t cmd = (counter_no << 6) | (read_write_latch << 4) || (mode << 1);  // 第0位为0表示二进制，为1表示BCD

    const uint8_t i8253_cmd_port = 0x43;
    const uint8_t i8253_data_port = 0x40 + counter_no;

    const uint32_t clock_rate = 1193180;
    uint16_t latch = (clock_rate + hz / 2) / hz;

    outb_p(cmd, i8253_cmd_port);
    outb_p(latch & 0xFF, i8253_data_port);
    outb(latch >> 8, i8253_data_port);
}
