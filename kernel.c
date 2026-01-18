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

/* Command shell buffers (global to avoid stack issues) */
static char cmd_buf[64];
static int cmd_pos = 0;
static uint8_t binary_buffer[65536];

/* Execute a command by loading and running a binary from the filesystem */
static void execute_command(const char *cmd) {
    /* Convert to uppercase for FAT32 8.3 filename */
    char filename[12];
    int i;
    for (i = 0; cmd[i] && i < 11; i++) {
        char ch = cmd[i];
        if (ch >= 'a' && ch <= 'z') {
            filename[i] = ch - 32;
        } else {
            filename[i] = ch;
        }
    }
    filename[i] = '\0';

    /* Try to open the file */
    fat32_file_t *file = fat32_open(filename);
    if (file) {
        if (file->size > 0 && file->size <= sizeof(binary_buffer)) {
            int bytes_read = fat32_read(file, binary_buffer, file->size);
            fat32_close(file);

            if (bytes_read > 0) {
                /* Check if it's an ELF file */
                if (binary_buffer[0] == 0x7f &&
                    binary_buffer[1] == 'E' &&
                    binary_buffer[2] == 'L' &&
                    binary_buffer[3] == 'F') {

                    syscall_init();
                    if (elf_load_and_exec(binary_buffer, bytes_read) != 0) {
                        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                        vga_puts("Failed to execute binary\n");
                        serial_puts(SERIAL_COM1, "Failed to execute binary\r\n");
                        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                    }
                } else {
                    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    vga_puts("Not an ELF binary\n");
                    serial_puts(SERIAL_COM1, "Not an ELF binary\r\n");
                    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                }
            } else {
                vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                vga_puts("Failed to read file\n");
                serial_puts(SERIAL_COM1, "Failed to read file\r\n");
                vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            }
        } else {
            fat32_close(file);
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_puts("File too large\n");
            serial_puts(SERIAL_COM1, "File too large\r\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
    } else {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_puts("Command not found: ");
        vga_puts(filename);
        vga_putchar('\n');
        serial_puts(SERIAL_COM1, "Command not found: ");
        serial_puts(SERIAL_COM1, filename);
        serial_puts(SERIAL_COM1, "\r\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
}

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
    vga_puts("System ready. Type a command or program name!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Show prompt */
    cmd_pos = 0;
    vga_puts("\nMagnOS> ");
    serial_puts(SERIAL_COM1, "\nMagnOS> ");

    while (1) {
        char c = 0;

        /* Check keyboard input */
        c = keyboard_getchar();

        /* Check serial input if no keyboard input */
        if (c == 0 && serial_received(SERIAL_COM1)) {
            c = serial_getchar(SERIAL_COM1);
            if (c == '\r') c = '\n';
        }

        if (c == 0) continue;

        if (c == '\b') {
            if (cmd_pos > 0) {
                cmd_pos--;
                vga_puts("\b \b");
                serial_putchar(SERIAL_COM1, '\b');
                serial_putchar(SERIAL_COM1, ' ');
                serial_putchar(SERIAL_COM1, '\b');
            }
        } else if (c == '\n') {
            vga_putchar('\n');
            serial_puts(SERIAL_COM1, "\r\n");

            cmd_buf[cmd_pos] = '\0';
            if (cmd_pos > 0) {
                execute_command(cmd_buf);
            }

            cmd_pos = 0;
            vga_puts("MagnOS> ");
            serial_puts(SERIAL_COM1, "MagnOS> ");
        } else if (cmd_pos < (int)sizeof(cmd_buf) - 1) {
            cmd_buf[cmd_pos++] = c;
            vga_putchar(c);
            serial_putchar(SERIAL_COM1, c);
        }
    }
}
