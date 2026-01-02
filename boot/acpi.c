/*
 * ------------------------------------------------------------------------
 *   File Name: acpi.c
 *      Author: Zhao Yanbai
 *              2025-12-28 16:56:11 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <multiboot2.h>
#include <string.h>
#include <system.h>
#include <page.h>
#include <ioremap.h>

typedef struct {
    char signature[8];   // "RSD PTR "
    uint8_t checksum;    // 校验和
    char oemid[6];       // OEM厂商ID
    uint8_t revision;    // ACPI版本（0=1.0，2=2.0）
    uint32_t rsdt_addr;  // 指向RSDT的物理地址
} __attribute__((packed)) rsdp_v1_t;

// ACPI标准表头（所有表都有）
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    char creator_id[4];
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

// RSDT（Root System Description Table）结构
typedef struct {
    acpi_sdt_header_t header;
    uint32_t table_ptrs[];  // 指向其他ACPI表的物理地址数组
} __attribute__((packed)) rsdt_t;

typedef struct {
    acpi_sdt_header_t header;
    uint32_t lapic_addr;  // 本地APIC地址
    uint32_t flags;
    // 后面跟着可变长度的结构
} __attribute__((packed)) madt_t;

void parse_madt(vaddr_t addr) {
    if (0 == addr) {
        printk("MADT addr is null\n");
        return;
    }

    printk("MADT vaddr %08x\n", addr);
    madt_t* madt = (madt_t*)addr;
    printk("MADT LAPIC addr %08x flags %08x\n", madt->lapic_addr, madt->flags);
    printk("MADT header size %u total length %u\n", sizeof(madt_t), madt->header.length);

    system.lapic_addr = madt->lapic_addr;

    uint8_t* ptr = (uint8_t*)(madt + 1);  // 跳过表头
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    uint32_t ioapic_cnt = 0;
    uint32_t lapic_cnt = 0;

    while (ptr < end) {
        uint8_t type = ptr[0];    // 条目类型
        uint8_t length = ptr[1];  // 条目长度

        if (0 == length) {
            printk("invalid madt entry\n");
            break;
        }

        switch (type) {
        case 0: {  // 本地APIC
            uint8_t cpu_id = ptr[2];
            uint8_t apic_id = ptr[3];
            uint32_t flags = *(uint32_t*)(ptr + 4);
            if (1 == flags) {
                lapic_cnt++;
                printk("LAPIC cpu %u apic %u flags %08x\n", cpu_id, apic_id, flags);
            } else {
                printk("LAPIC cpu %u apic %u flags %08x [disabled]\n", cpu_id, apic_id, flags);
            }
        } break;
        case 1: {  // IO APIC
            uint8_t ioapic_id = ptr[2];
            uint8_t reserved = ptr[3];
            uint32_t ioapic_addr = *(uint32_t*)(ptr + 4);
            uint32_t global_irq_base = *(uint32_t*)(ptr + 8);
            ioapic_cnt++;
            if (ioapic_cnt == 1) {
                system.ioapic_addr = ioapic_addr;
            } else {
                // 多个IO-APIC，就不支持了
                panic("multiple IO-APIC not supported\n");
            }
            printk("IOAPIC id %u addr %08x global_irq_base %u\n", ioapic_id, ioapic_addr, global_irq_base);
        } break;
        case 2: {  // IO APIC 中断源重映射
            uint8_t bus = ptr[2];
            uint8_t irq = ptr[3];
            uint32_t global_irq = *(uint32_t*)(ptr + 4);
            uint16_t flags = *(uint16_t*)(ptr + 8);
            printk("IOAPIC irq %u global_irq %u flags %04x bus %u\n", irq, global_irq, flags, bus);
        } break;
        case 3: {  // 本地APIC非统一中断
            uint8_t cpu_id = ptr[2];
            uint8_t apic_id = ptr[3];
            uint8_t flags = ptr[4];
            printk("LAPIC non-uniform cpu %u apic %u flags %02x\n", cpu_id, apic_id, flags);
        } break;
        case 4: {  // 虚拟化APIC
            uint16_t reserved = *(uint16_t*)(ptr + 2);
            uint32_t apic_addr = *(uint32_t*)(ptr + 4);
            printk("Virtual APIC addr %08x\n", apic_addr);
        } break;
        default:
            printk("unknown madt entry type %u\n", type);
            break;
        }
        ptr += length;
    }
}

void parse_rsdt(paddr_t addr) {
    if (0 == addr) {
        printk("RSDT addr is null\n");
        return;
    }

    printk("parse rsdt\n");
    rsdt_t* rsdt = (rsdt_t*)(ioremap(PAGE_ALIGN(addr), PAGE_SIZE) + (addr - PAGE_ALIGN(addr)));
    assert(rsdt != NULL);

    // 验证签名
    if (memcmp(rsdt->header.signature, "RSDT", 4) != 0) {
        panic("ACPI RSDT invalid\n");
    }

    // 计算表指针数量
    // 表总长度 - 表头长度 = 指针数组长度
    uint32_t header_size = sizeof(acpi_sdt_header_t);
    uint32_t pointers_size = rsdt->header.length - header_size;
    uint32_t table_count = pointers_size / sizeof(uint32_t);

    printk("RSDT have %u ACPI tables\n", table_count);

    for (int i = 0; i < table_count; i++) {
        uint32_t table_phys_addr = rsdt->table_ptrs[i];
        // TODO unioremap
        // printk("ACPI table %u addr %08x\n", i, table_phys_addr);
        acpi_sdt_header_t* table = (acpi_sdt_header_t*)(ioremap(PAGE_ALIGN(table_phys_addr), PAGE_SIZE) +
                                                        (table_phys_addr - PAGE_ALIGN(table_phys_addr)));
        if (table == 0) {
            printk("ACPI table %u:%x is null\n", i, table_phys_addr);
            continue;
        }

        char sig[5] = {0};
        memcpy(sig, table->signature, 4);
        if (sig[0] == 0) {
            continue;
        }
        printk("ACPI table %u signature %s addr %08x len %u\n", i, sig, table_phys_addr, table->length);
        if (memcmp(sig, "APIC", 4) == 0) {
            printk("found MADT table, length %u\n", table->length);
            parse_madt((vaddr_t)table);
        }
    }
}

void init_acpi() {
    parse_rsdt((paddr_t)system.rsdt_addr);
    // asm("cli;hlt;");
}

void parse_acpi(void* tag) {
    printk("ACPI[old].RSDP ");
    struct multiboot_tag_old_acpi* acpi_tag = (struct multiboot_tag_old_acpi*)tag;
    uint8_t* rsdp = (uint8_t*)acpi_tag->rsdp;
    for (int i = 0; i < acpi_tag->size; i++) {
        printk("%02x ", rsdp[i]);
    }
    printk("\n");

    if (memcmp(rsdp, "RSD PTR ", 8) != 0) {
        panic("ACPI[old] RSDP invalid\n");
    }
    rsdp_v1_t* rsdp_v1 = (rsdp_v1_t*)rsdp;
    printk("ACPI[old] RSDP checksum %02x\n", rsdp_v1->checksum);
    printk("ACPI[old] RSDP OEM ID %6s\n", rsdp_v1->oemid);
    printk("ACPI[old] RSDP revision %u\n", rsdp_v1->revision);
    printk("ACPI[old] RSDP RSDT addr %08x\n", rsdp_v1->rsdt_addr);

    system.rsdt_addr = (void*)rsdp_v1->rsdt_addr;
}
