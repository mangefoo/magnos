#include "vga.h"
#include "serial.h"

/* Kernel main function */
void kernel_main(void) {
    /* Ensure interrupts are disabled (no IDT yet) */
    __asm__ volatile("cli");

    /* Initialize VGA */
    vga_init();

    /* Initialize serial port COM1 */
    serial_init(SERIAL_COM1);

    /* Display welcome message on VGA */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("MagnOS v0.1\n");
    vga_puts("===========\n\n");

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Bootable x86 Operating System\n");
    vga_puts("VGA Driver: OK\n");
    vga_puts("Serial Driver: OK\n\n");

    /* Send message to serial port */
    serial_puts(SERIAL_COM1, "MagnOS v0.1 - Serial port initialized\n");
    serial_puts(SERIAL_COM1, "Kernel loaded successfully\n");

    /* Demonstrate color output */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("System initialized successfully!\n\n");

    /* Display some system info */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Kernel at: ");
    vga_puthex((uint32_t)kernel_main);
    vga_puts("\n");

    vga_puts("VGA at: ");
    vga_puthex(VGA_MEMORY);
    vga_puts("\n");

    vga_puts("Serial COM1: ");
    vga_puthex(SERIAL_COM1);
    vga_puts("\n\n");

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("System ready. Type in terminal for serial echo!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Echo loop - read from serial and echo back */
    serial_puts(SERIAL_COM1, "\nMagnOS> ");

    while (1) {
        if (serial_received(SERIAL_COM1)) {
            char c = serial_getchar(SERIAL_COM1);

            /* Echo to serial */
            serial_putchar(SERIAL_COM1, c);

            /* Also display on VGA */
            vga_putchar(c);

            /* Handle enter key */
            if (c == '\r' || c == '\n') {
                serial_puts(SERIAL_COM1, "\nMagnOS> ");
            }
        }
    }
}
