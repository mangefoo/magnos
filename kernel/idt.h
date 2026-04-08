#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT entry (8 bytes) */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

/* IDT pointer for lidt */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* Register state pushed by ISR stubs, passed to C handler */
struct isr_regs {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
};

#define IDT_ENTRIES 256
#define IDT_GATE_INT32 0x8E  /* Present, ring 0, 32-bit interrupt gate */

void idt_init(void);

extern volatile uint32_t pit_ticks;

/* Sleep for approximately the given number of milliseconds */
void sleep_ms(uint32_t ms);

/* Get system uptime in milliseconds since boot */
uint32_t get_uptime_ms(void);

#endif /* IDT_H */
