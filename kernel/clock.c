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

unsigned int sys_clock() {
    return jiffies;
}

void clk_bh_handler(void* arg);

extern volatile bool enable_clock_irq_delay;

void clk_handler(unsigned int irq, pt_regs_t* regs, void* dev_id) {
    jiffies++;

    current->jiffies = jiffies;

#if ENABLE_CLOCK_IRQ_WAIT
    if (enable_clock_irq_delay) {
        return;
    }
    enable_clock_irq_delay = true;
#endif

    current->ticks--;

    if (reenter == 0) {
        add_irq_bh_handler(clk_bh_handler, NULL);
    }
}

// 开中断执行这个函数
// 后续放到一个内核任务中去做，需要先把禁止内核抢占做了
const char* task_state(unsigned int state);
void clk_bh_handler(void* arg) {
    task_t* p = 0;
    list_head_t* t = 0;
    list_head_t* pos = 0;
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
}

uint16_t read_i8254_counter(uint8_t counter_no) {
    const uint8_t i8254_cmd_port = 0x43;
    const uint8_t i8254_data_port = 0x40 + counter_no;

    assert(counter_no < 3);

    uint8_t cmd = 0x00;
    cmd |= 0x03 << 6;                 // read back command
    cmd |= 0x01 << 4;                 // don't latch status
    cmd |= 0x01 << (counter_no + 1);  // read back timer channel n

    uint16_t value = 0;

    outb_p(cmd, i8254_cmd_port);

    value |= inb(i8254_data_port) << 0;
    value |= inb(i8254_data_port) << 8;

    return value;
}

void setup_i8254(uint16_t hz) {
    // PC/AT 8254的连接方法
    // 第0个计数器连连到了8259A的IRQ0
    // 第1个计数器连连到了DRAM刷新电路，通常情况下，该通道的频率设置为15.2kHz
    // 第2个计数器连连到了音频驱动单元，可以通过频率控制发声

    // 8254的最低频率为18.2Hz(1193180/65536，最大计数值是65536是因为往计数器里写0就从65536开始计数)
    assert(hz >= 19);

    const uint8_t counter_no = 0;        // 第0个计数器
    const uint8_t read_write_latch = 3;  // 0 锁存数据供CPU读；1只读写低字节；2只读写高字节；3先读写低字节，后读写高字节
    const uint8_t mode = 2;              //
    const uint8_t BCD = 0;               // 0 二进制；1 BCD码

    const uint8_t cmd =
        ((counter_no & 0x03) << 6) | ((read_write_latch & 0x03) << 4) | ((mode & 0x07) << 1) | ((BCD & 0x01) << 0);

    const uint8_t i8254_cmd_port = 0x43;
    const uint8_t i8254_data_port = 0x40 + counter_no;

    const uint32_t clock_rate = 1193180;
    uint16_t latch = (clock_rate + hz / 2) / hz;

    // 必须先写控制命令，再写入初始计数值
    outb_p(cmd, i8254_cmd_port);
    outb_p((latch >> 0) & 0xFF, i8254_data_port);
    outb_p((latch >> 8) & 0xFF, i8254_data_port);

    // for (uint8_t i = 0; i < 3; i++) {
    //     printk("i8254 counter%u value %u\n", i, read_i8254_counter(i));
    // }
}
