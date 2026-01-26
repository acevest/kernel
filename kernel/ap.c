/*
 * ------------------------------------------------------------------------
 *   File Name: ap.c
 *      Author: Zhao Yanbai
 *              2026-01-04 20:15:33 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <page.h>
#include <types.h>
#include <system.h>
#include <processor.h>
#include <linkage.h>
#include <msr.h>
#include <cpuid.h>
#include <hpet.h>
#include <disk.h>

extern pde_t* ap_pre_pgd;

#define AP_GDT_CNT 8
#define AP_IDT_CNT 256

desc_t ALIGN8 ap_idt[AP_IDT_CNT];
uint8_t ALIGN8 ap_idtr[6];

#define AP_CS_SELECTOR 0x08
#define AP_DS_SELECTOR 0x10

const uint32_t ap_pit_irq = 0x00;
const uint32_t ap_lapic_irq = 0x08;

static inline desc_t _create_ap_gate(u32 handler, u8 type, u8 DPL) {
    desc_t d;
    gate_t* p = &d.gate;

    _init_desc(&d);

    p->eaddrL = 0xFFFF & handler;
    p->eaddrH = 0xFFFF & (handler >> 16);
    p->selector = AP_CS_SELECTOR;
    p->type = type;
    p->P = 0x1;
    p->DPL = DPL;

    return d;
}

static inline void set_ap_idt_gate(u32 vec, u32 handler, u8 type, u8 DPL) {
    ap_idt[vec] = _create_ap_gate(handler, type, DPL);
}

#define set_ap_sys_int(vect, type, DPL, handler)        \
    do {                                                \
        void handler();                                 \
        set_ap_idt_gate(vect, (u32)handler, type, DPL); \
    } while (0)

uint32_t ap_get_kernel_esp() {
    // 虽然ap_pre_pgd只做了一下跳板页目录看起来很可惜
    // 不过可以把它拿来个AP的栈用
    return (uint32_t)pa2va(&ap_pre_pgd) + PAGE_SIZE;
}

void ap_kernel_entry() {
    // 进入内核空间后要加载原来已经在内核空间的ap_gdt
    // 之前加载的被复制到1MB以下的ap_gdt需要废弃，因为它已经在内核地址空间之外了
    // 虽然他们是同一个ap_gdt
    extern char ap_gdtr;
    // 最简单的写 asm("lgdt ap_gdtr"); 但是c编译器不通过
    asm("lgdt %0" ::"m"(ap_gdtr));

    // tss
    // AP 暂时不服务用户特权级 所以暂时用不到tss

    //
    set_ap_sys_int(0x00, TRAP_GATE, PRIVILEGE_KRNL, APDivideError);
    set_ap_sys_int(0x01, TRAP_GATE, PRIVILEGE_KRNL, APDebug);
    set_ap_sys_int(0x02, INTR_GATE, PRIVILEGE_KRNL, APNMI);
    set_ap_sys_int(0x03, TRAP_GATE, PRIVILEGE_USER, APBreakPoint);
    set_ap_sys_int(0x04, TRAP_GATE, PRIVILEGE_USER, APOverFlow);
    set_ap_sys_int(0x05, TRAP_GATE, PRIVILEGE_USER, APBoundsCheck);
    set_ap_sys_int(0x06, TRAP_GATE, PRIVILEGE_KRNL, APInvalidOpcode);
    set_ap_sys_int(0x07, TRAP_GATE, PRIVILEGE_KRNL, APDeviceNotAvailable);
    set_ap_sys_int(0x08, TRAP_GATE, PRIVILEGE_KRNL, APDoubleFault);
    set_ap_sys_int(0x09, TRAP_GATE, PRIVILEGE_KRNL, APCoprocSegOverRun);
    set_ap_sys_int(0x0A, TRAP_GATE, PRIVILEGE_KRNL, APInvalidTss);
    set_ap_sys_int(0x0B, TRAP_GATE, PRIVILEGE_KRNL, APSegNotPresent);
    set_ap_sys_int(0x0C, TRAP_GATE, PRIVILEGE_KRNL, APStackFault);
    set_ap_sys_int(0x0D, TRAP_GATE, PRIVILEGE_KRNL, APGeneralProtection);
    set_ap_sys_int(0x0E, TRAP_GATE, PRIVILEGE_KRNL, APPageFault);
    set_ap_sys_int(0x10, TRAP_GATE, PRIVILEGE_KRNL, APCoprocError);

    // idt gates
    for (int i = 0x11; i < 0x20; i++) {
        set_ap_sys_int(i, INTR_GATE, PRIVILEGE_KRNL, ap_no_irq_handler);
    }

    // irq gates
    for (int i = 0x20; i < 256; i++) {
        set_ap_sys_int(i, INTR_GATE, PRIVILEGE_KRNL, ap_no_irq_handler);
    }

    set_ap_sys_int(0x20 + ap_pit_irq, INTR_GATE, PRIVILEGE_KRNL, ap_pit_irq_handler);
    set_ap_sys_int(0x20 + ap_lapic_irq, INTR_GATE, PRIVILEGE_KRNL, ap_lapic_irq_handler);

    // 加载 ap_idtr
    *((unsigned short*)ap_idtr) = AP_IDT_CNT * sizeof(gate_t);
    *((unsigned long*)(ap_idtr + 2)) = (unsigned long)ap_idt;
    asm volatile("lidt ap_idtr");

    cpuid_regs_t r;
    r = cpuid(1);
    printk("cpuid 1: eax=%08x ebx=%08x ecx=%08x edx=%08x\n", r.eax, r.ebx, r.ecx, r.edx);
    if (r.edx & (1 << 9)) {
        printk("local apic supported\n");
        if (r.ecx & (1 << 21)) {
            printk("x2apic supported\n");
        } else {
            printk("x2apic not supported\n");
        }
    } else {
        panic("local apic not supported\n");
    }

    // 开房时钟中断
    lapic_t* lapic = system.lapic;

    uint32_t apic_base = 0;
    apic_base = read_msr32(MSR_IA32_APIC_BASE);
    apic_base |= (1 << 11);
    if (lapic->x2apic) {
        apic_base |= (1 << 10);
        write_msr(MSR_IA32_APIC_BASE, apic_base);
    }

    uint32_t lapic_svr = lapic->read(LAPIC_SVR);
    lapic_svr |= (1 << 8);    // 启用LAPIC
    lapic_svr &= ~(1 << 12);  // 禁用EOI广播
    lapic->write(LAPIC_SVR, lapic_svr);
#if 1
    uint32_t calibration_counter = 0;
    uint32_t calibration_divider = 0;
    struct {
        uint32_t value;
        const char* name;
    } divisors[] = {
        {LAPIC_TIMER_DIVIDE_BY_1, "1"},           //
        {LAPIC_TIMER_DIVIDE_BY_2, "2"},           //
        {LAPIC_TIMER_DIVIDE_BY_4, "4"},           //
        {LAPIC_TIMER_DIVIDE_BY_8, "8"},           //
        {LAPIC_TIMER_DIVIDE_BY_16, "16"},         //
        {LAPIC_TIMER_DIVIDE_BY_32, "32"},         //
        {LAPIC_TIMER_DIVIDE_BY_64, "64"},         //
        {LAPIC_TIMER_DIVIDE_BY_128, "128"},       //
        {LAPIC_TIMER_DIVIDE_INVALID, "invalid"},  // END
    };

    lapic->write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    for (int i = 0; divisors[i].value != LAPIC_TIMER_DIVIDE_INVALID; i++) {
        const uint32_t timn = 0;

        //
        calibration_divider = divisors[i].value;
        lapic->write(LAPIC_TIMER_DIVIDER, calibration_divider);
        uint32_t divide = lapic->read(LAPIC_TIMER_DIVIDER);
        assert(divide == calibration_divider);

        //
        hpet_disable();

        // 只要不hpet_enable, HPET的计数器就不会启动
        hpet_prepare_calibration(timn, 97 /*hz*/);

        //
        hpet_enable();

        // 写入最大值
        lapic->write(LAPIC_TIMER_INITIAL, 0xFFFFFFFF);

        uint32_t lapic_timer_counter = 0;
        while (!hpet_calibration_end(timn)) {
            lapic_timer_counter = lapic->read(LAPIC_TIMER_COUNTER);
            if (unlikely(lapic_timer_counter < 0x80000000)) {
                goto next;
            } else {
                asm("pause");
            }
        }

        lapic_timer_counter = lapic->read(LAPIC_TIMER_COUNTER);

        calibration_counter = 0xFFFFFFFF - lapic_timer_counter;
        assert(lapic_timer_counter >= 0x80000000);
        printk("AP chose divider: %s %02x\n", divisors[i].name, divisors[i].value);
        printk("AP calibration counter: %08x\n", calibration_counter);
        break;

    next:
        continue;
    }

    if (calibration_counter == 0) {
        panic("AP calibration counter is 0\n");
    }

    //
    hpet_disable();

    // 关闭时钟中断
    lapic->write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);

    // 设置时钟中断周期
    lapic->write(LAPIC_TIMER_DIVIDER, calibration_divider);
    lapic->write(LAPIC_TIMER_INITIAL, calibration_counter);

    //
    lapic->write(LAPIC_LVT_TIMER, LAPIC_TIMER_MODE_PERIODIC | (0x20 + ap_lapic_irq));
#endif

    uint32_t ap_cpuid = lapic->get_lapic_id();
    printk("AP CPU id: %d\n", ap_cpuid);
    system.ap_cpuid = ap_cpuid;  // 这之后不能再printk了
    asm("sti;");

    const char* title = "KERNEL MONITOR";
    printlxy(MPL_TITLE, (80 - strlen(title)) / 2, title);

    void system_monitor();
    system_monitor();
}

bool ap_ready() {
    return system.ap_cpuid != 0;
}

uint64_t ap_pit_ticks = 0;
void do_ap_pit_irq_handler() {
    uint8_t* p = (uint8_t*)0xC00B8004;
    *p = *p == ' ' ? 'R' : ' ';

    ap_pit_ticks++;

    ioapic_eoi();
}

void do_ap_no_irq_handler() {
    panic("AP unexpected irq\n");
}

uint64_t ap_lapic_ticks = 0;
void do_ap_lapic_irq_handler() {
    uint8_t* p = (uint8_t*)0xC00B8002;
    *p = *p == ' ' ? 'E' : ' ';

    ap_lapic_ticks++;

    ioapic_eoi();
}

void _system_monitor() {
    //
    extern volatile uint64_t jiffies;
    // extern volatile uint64_t hpet_ticks;  // jiffies 和 hpet_ticks 是同一个东西
    printlxy(MPL_IRQ, MPO_HPET, "HPET: %lu", jiffies);

    //
    extern volatile uint8_t kbd_scan_code;
    extern volatile uint64_t kbd_irq_cnt;
    printlxy(MPL_IRQ, MPO_KEYBOARD, "KBD: %02x %lu", kbd_scan_code, kbd_irq_cnt);

    //
    printlxy(MPL_IRQ, MPO_AP_CLOCK, "AP: %lu", ap_lapic_ticks);

    extern disk_request_queue_t disk_request_queue;
    disk_request_queue_t* drq = &disk_request_queue;
    printlxy(MPL_IRQ, MPO_DISK, "DISK: %lu/%u/%lu", drq->req_count, drq->pending_count, drq->completed_count);

    //
    void print_all_tasks();
    print_all_tasks();

    //
    // void print_all_ides();
    // print_all_ides();
}
void system_monitor() {
    while (1) {
        asm("hlt");
        uint64_t ap_monitor_begin = ap_lapic_ticks;
        _system_monitor();
        uint64_t ap_monitor_end = ap_lapic_ticks;
        if (ap_monitor_end != ap_monitor_begin) {
            // panic("AP monitor cost too long\n");
        }
    }
}

#include <task.h>
const char* task_state(unsigned int state) {
    static const char s[][8] = {
        " ERROR", "\x10\x07RUN\x07", " READY", " WAIT ", " INIT ", " EXIT ",
    };

    if (state >= TASK_END) {
        state = TASK_UNUSED;
    }

    return s[state];
}
void print_all_tasks() {
    extern task_t* monitor_tasks[];

    printl(MPL_TASK_TITLE, "         NAME      STATE TK/PI REASON     SCHED     KEEP");

    for (int i = 0; i < 10; i++) {
        task_t* p = monitor_tasks[i];

        if (p == NULL) {
            continue;
        }

        printl(MPL_TASK_0 + p->pid, "%08x %-6s:%u %s %02d/%02u %-10s %-9u %-9u",
               p,                     //
               p->name,               //
               p->pid,                //
               task_state(p->state),  //
               p->ticks,              //
               p->priority,           //
               p->reason,             //
               p->sched_cnt,          //
               p->sched_keep_cnt      //
        );
    }
}

#include <ide.h>
extern ide_pci_controller_t ide_pci_controller[];
void ide_stat_print(ide_pci_controller_t* ide_ctrl) {
    assert(ide_ctrl != NULL);
    assert(ide_ctrl >= ide_pci_controller);
    assert(ide_ctrl < (ide_pci_controller + NR_IDE_CONTROLLER));
    int r = atomic_read(&(ide_ctrl->request_cnt));
    int i = atomic_read(&(ide_ctrl->irq_cnt));
    int c = atomic_read(&(ide_ctrl->consumed_cnt));
    int channel = ide_ctrl->channel;
    printlxy(MPL_IDE0 + channel, MPO_IDE, "IDE%d req %u irq %u consumed %u", channel, r, i, c);
}

void print_all_ides() {
    ide_stat_print(&ide_pci_controller[0]);
    ide_stat_print(&ide_pci_controller[1]);
}
