#include "gdt.h"

/* GDT entry */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

/* GDT pointer for lgdt */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define GDT_ENTRIES 6

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdtp;
static struct tss_entry tss;

static void gdt_set_gate(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[idx].base_low    = base & 0xFFFF;
    gdt[idx].base_mid    = (base >> 16) & 0xFF;
    gdt[idx].base_high   = (base >> 24) & 0xFF;
    gdt[idx].limit_low   = limit & 0xFFFF;
    gdt[idx].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[idx].access      = access;
}

/* Defined in gdt_flush.asm */
extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

void gdt_init(void) {
    /* Zero the TSS */
    uint8_t *p = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss); i++)
        p[i] = 0;

    /* Set TSS kernel stack */
    tss.ss0 = GDT_KERNEL_DATA;
    tss.esp0 = 0x1F0000;
    tss.iomap_base = sizeof(tss);

    /* Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Kernel code: base=0, limit=4GB, ring 0, executable, readable */
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);

    /* Kernel data: base=0, limit=4GB, ring 0, writable */
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);

    /* User code: base=0, limit=4GB, ring 3, executable, readable */
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF);

    /* User data: base=0, limit=4GB, ring 3, writable */
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);

    /* TSS descriptor */
    uint32_t tss_base = (uint32_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00);

    /* Load GDT */
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint32_t)&gdt;
    gdt_flush((uint32_t)&gdtp);
    tss_flush();
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}
