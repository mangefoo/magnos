#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "heap.h"
#include "paging.h"
#include "process.h"
#include "framebuffer.h"
#include "mouse.h"
#include "gui.h"
#include "calc.h"

static void log_init(const char *name, int ok) {
    serial_puts(SERIAL_COM1, name);
    serial_puts(SERIAL_COM1, ok ? ": OK\r\n" : ": FAILED\r\n");
}

void kernel_main(void) {
    serial_init(SERIAL_COM1);
    serial_puts(SERIAL_COM1, "\r\n=== MagnOS booting (GUI mode) ===\r\n");

    gdt_init();
    log_init("GDT/TSS", 1);

    /* Keyboard not needed by the calculator, but the controller
     * commands the mouse driver issues live on the same chip. */
    idt_init();
    log_init("IDT/Interrupts", 1);

    pmm_init();
    log_init("PMM", 1);

    heap_init();
    log_init("Heap", 1);

    paging_init();
    log_init("Paging", 1);

    process_init();
    log_init("Processes", 1);

    /* Framebuffer comes up after paging so we can map the LFB. */
    int fb_ok = (fb_init() == 0);
    log_init("Framebuffer", fb_ok);

    if (!fb_ok) {
        serial_puts(SERIAL_COM1, "VBE mode not set by bootloader — halting.\r\n");
        for (;;) __asm__ volatile("cli; hlt");
    }

    /* Paint a splash so the screen isn't blank if the mouse takes a moment. */
    fb_clear(FB_DESKTOP);
    fb_draw_string(20, 20, "MagnOS booting...", FB_WHITE, 2);

    mouse_init();
    log_init("PS/2 Mouse", 1);

    gui_init();
    log_init("GUI", 1);

    serial_puts(SERIAL_COM1, "Launching calculator.\r\n");
    calc_run();

    /* calc_run never returns; this is just defensive. */
    for (;;) __asm__ volatile("cli; hlt");
}
