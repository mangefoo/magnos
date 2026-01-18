#include "vga.h"
#include "serial.h"
#include "ide.h"
#include "fat32.h"
#include "elf.h"
#include "syscall.h"
#include "keyboard.h"

/* Feature flags */
#define PRINT_HELLO_TXT      0
#define ENABLE_HELLO_BINARY  0

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

    /* Initialize keyboard */
    keyboard_init();
    vga_puts("Keyboard Driver: OK\n");

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

#if PRINT_HELLO_TXT
        /* Try to read hello.txt */
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_puts("Reading HELLO.TXT...\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        fat32_file_t *file = fat32_open("HELLO.TXT");
        if (file) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_puts("File found! Contents:\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_puts("---\n");

            /* Read file contents (max 512 bytes for now) */
            uint8_t buffer[512];
            int bytes_read = fat32_read(file, buffer, sizeof(buffer) - 1);

            if (bytes_read > 0) {
                /* Null terminate */
                buffer[bytes_read] = '\0';

                /* Print contents */
                for (int i = 0; i < bytes_read; i++) {
                    vga_putchar(buffer[i]);
                }
            } else {
                vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                vga_puts("Failed to read file\n");
            }

            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_puts("\n---\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

            fat32_close(file);
        } else {
            vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
            vga_puts("File not found\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }

        vga_puts("\n");
#endif

#if ENABLE_HELLO_BINARY
        /* Try to load and execute HELLO binary */
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_puts("Loading HELLO binary...\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        fat32_file_t *bin_file = fat32_open("HELLO");
        if (bin_file) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_puts("Binary found! Size: ");
            vga_puthex(bin_file->size);
            vga_puts(" bytes\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

            /* Allocate buffer for binary (max 64KB) */
            static uint8_t binary_buffer[65536];

            if (bin_file->size > sizeof(binary_buffer)) {
                vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                vga_puts("Binary too large!\n");
                vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            } else {
                /* Read entire binary */
                int bytes_read = fat32_read(bin_file, binary_buffer, bin_file->size);

                if (bytes_read > 0) {
                    vga_puts("Binary loaded. ");

                    /* Initialize syscalls */
                    syscall_init();

                    /* Load and execute ELF binary */
                    if (elf_load_and_exec(binary_buffer, bytes_read) == 0) {
                        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
                        vga_puts("\nBinary execution successful!\n");
                        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                    } else {
                        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                        vga_puts("Failed to execute binary\n");
                        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                    }
                } else {
                    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    vga_puts("Failed to read binary\n");
                    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                }
            }

            fat32_close(bin_file);
        } else {
            vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
            vga_puts("Binary not found (add HELLO file to disk)\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }

        vga_puts("\n");
#endif
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("FAILED\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_puts("(Disk may not be formatted as FAT32)\n\n");
    }

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("System ready. Type on keyboard or serial!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Show prompt */
    vga_puts("\nMagnOS> ");
    serial_puts(SERIAL_COM1, "\nMagnOS> ");

    while (1) {
        /* Check keyboard input */
        char c = keyboard_getchar();
        if (c != 0) {
            if (c == '\b') {
                /* Handle backspace */
                vga_puts("\b \b");
                serial_putchar(SERIAL_COM1, '\b');
                serial_putchar(SERIAL_COM1, ' ');
                serial_putchar(SERIAL_COM1, '\b');
            } else {
                /* Echo to VGA */
                vga_putchar(c);

                /* Echo to serial */
                serial_putchar(SERIAL_COM1, c);

                /* Handle enter key */
                if (c == '\n') {
                    vga_puts("MagnOS> ");
                    serial_puts(SERIAL_COM1, "MagnOS> ");
                }
            }
        }

        /* Check serial input */
        if (serial_received(SERIAL_COM1)) {
            c = serial_getchar(SERIAL_COM1);

            /* Echo to serial */
            serial_putchar(SERIAL_COM1, c);

            /* Also display on VGA */
            vga_putchar(c);

            /* Handle enter key */
            if (c == '\r' || c == '\n') {
                vga_puts("MagnOS> ");
                serial_puts(SERIAL_COM1, "\nMagnOS> ");
            }
        }
    }
}
