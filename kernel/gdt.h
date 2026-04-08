#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Segment selectors */
#define GDT_KERNEL_CODE  0x08
#define GDT_KERNEL_DATA  0x10
#define GDT_USER_CODE    0x18
#define GDT_USER_DATA    0x20
#define GDT_TSS          0x28

/* TSS structure */
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;      /* Kernel stack pointer */
    uint32_t ss0;       /* Kernel stack segment */
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/* Initialize GDT with kernel/user segments and TSS */
void gdt_init(void);

/* Update the kernel stack pointer in the TSS (used during context switch) */
void tss_set_kernel_stack(uint32_t esp0);

#endif /* GDT_H */
