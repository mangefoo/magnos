#include "idt.h"
#include "io.h"
#include "vga.h"
#include "keyboard.h"

/* IDT table and pointer */
static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

/* PIT tick counter */
volatile uint32_t pit_ticks = 0;

/* ISR stub declarations from isr.asm */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void); extern void isr32(void);
extern void isr33(void); extern void isr34(void); extern void isr35(void);
extern void isr36(void); extern void isr37(void); extern void isr38(void);
extern void isr39(void); extern void isr40(void); extern void isr41(void);
extern void isr42(void); extern void isr43(void); extern void isr44(void);
extern void isr45(void); extern void isr46(void); extern void isr47(void);

/* Exception names for diagnostic output */
static const char *exception_names[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD FPU Exception",
    "Virtualization Exception",
    "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved"
};

/* Set one IDT gate */
static void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t type_attr) {
    idt[num].offset_low  = handler & 0xFFFF;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
    idt[num].selector    = selector;
    idt[num].zero        = 0;
    idt[num].type_attr   = type_attr;
}

/* Remap the 8259 PIC: IRQ0-7 → INT 32-39, IRQ8-15 → INT 40-47 */
static void pic_remap(void) {
    /* Save masks */
    uint8_t mask1 = inb(0x21);
    uint8_t mask2 = inb(0xA1);

    /* ICW1: begin initialization (cascade, ICW4 needed) */
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();

    /* ICW2: vector offsets */
    outb(0x21, 0x20); io_wait();  /* PIC1: IRQ0 → INT 32 */
    outb(0xA1, 0x28); io_wait();  /* PIC2: IRQ8 → INT 40 */

    /* ICW3: cascade wiring */
    outb(0x21, 0x04); io_wait();  /* PIC1: slave on IRQ2 */
    outb(0xA1, 0x02); io_wait();  /* PIC2: cascade identity 2 */

    /* ICW4: 8086 mode */
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    /* Restore saved masks */
    outb(0x21, mask1);
    outb(0xA1, mask2);
}

/* Program PIT channel 0 for ~100 Hz */
static void pit_init(void) {
    uint16_t divisor = 11932;  /* 1193180 / 100 ≈ 11932 */
    outb(0x43, 0x36);          /* Channel 0, lobyte/hibyte, rate generator */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

/* C interrupt dispatcher — called from isr_common_stub */
void isr_handler(struct isr_regs *regs) {
    if (regs->int_no < 32) {
        /* CPU exception */
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("\n*** EXCEPTION: ");
        vga_puts(exception_names[regs->int_no]);
        vga_puts(" ***\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_puts("EIP: ");
        vga_puthex(regs->eip);
        vga_puts("  Error code: ");
        vga_puthex(regs->err_code);
        vga_puts("\n");

        /* Halt on exception */
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("System halted.\n");
        __asm__ volatile("cli; hlt");
    }

    if (regs->int_no == 32) {
        /* IRQ0: PIT timer tick */
        pit_ticks++;
    } else if (regs->int_no == 33) {
        /* IRQ1: Keyboard */
        keyboard_irq_handler();
    }

    /* Send EOI to PIC */
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);  /* EOI to PIC2 */
    }
    if (regs->int_no >= 32) {
        outb(0x20, 0x20);  /* EOI to PIC1 */
    }
}

/* Sleep for approximately the given number of milliseconds */
void sleep_ms(uint32_t ms) {
    uint32_t ticks_to_wait = ms / 10;  /* 100 Hz = 10ms per tick */
    if (ticks_to_wait == 0) ticks_to_wait = 1;
    uint32_t target = pit_ticks + ticks_to_wait;
    while (pit_ticks < target) {
        __asm__ volatile("hlt");  /* Sleep until next interrupt */
    }
}

/* Get system uptime in milliseconds since boot */
uint32_t get_uptime_ms(void) {
    return pit_ticks * 10;  /* 100 Hz = 10ms per tick */
}

/* Initialize IDT, PIC, PIT, and enable interrupts */
void idt_init(void) {
    /* ISR stub table */
    void (*isrs[48])(void) = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
        isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
        isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
    };

    /* Register all 48 ISR stubs */
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, (uint32_t)isrs[i], 0x08, IDT_GATE_INT32);
    }

    /* Load IDT */
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    __asm__ volatile("lidt (%0)" : : "r"(&idtp));

    /* Remap PIC */
    pic_remap();

    /* Configure PIT */
    pit_init();

    /* Unmask IRQ0 (timer) and IRQ1 (keyboard), mask all others */
    outb(0x21, 0xFC);  /* PIC1: 11111100 — only IRQ0 and IRQ1 enabled */
    outb(0xA1, 0xFF);  /* PIC2: all masked */

    /* Enable interrupts */
    __asm__ volatile("sti");
}
