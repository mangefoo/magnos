#include "vga.h"
#include "serial.h"
#include "ide.h"
#include "fat32.h"

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
    vga_puts("Serial Driver: OK\n");

    /* Initialize IDE */
    vga_puts("IDE Driver: ");
    if (ide_init() == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("OK\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("FAILED\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
    vga_puts("\n");

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

    /* Initialize FAT32 filesystem */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Initializing FAT32 Filesystem...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    vga_puts("FAT32: ");
    if (fat32_init(0) == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("OK\n\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        /* List files */
        fat32_list_root();
        vga_puts("\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("FAILED\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_puts("(Disk may not be formatted as FAT32)\n\n");
    }

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
