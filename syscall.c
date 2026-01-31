#include "syscall.h"
#include "vga.h"
#include "elf.h"
#include "fat32.h"
#include "serial.h"
#include "args.h"
#include "keyboard.h"

/* Memory functions */
static uint32_t strlen(const char *str) {
    uint32_t len = 0;
    while (str[len]) len++;
    return len;
}

/* Currently open file (only one file at a time) */
static fat32_file_t *current_file = NULL;

/* Syscall handler */
uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    (void)arg3;  /* Unused */

    switch (syscall_num) {
        case SYSCALL_PRINT: {
            /* arg1 = pointer to string */
            const char *str = (const char *)arg1;
            if (str) {
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_puts(str);
                serial_puts(SERIAL_COM1, str);
            }
            return 0;
        }

        case SYSCALL_EXIT: {
            /* arg1 = exit code */
            vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
            vga_puts("\n[Program exited with code: ");
            vga_puthex(arg1);
            vga_puts("]\n");
            /* Return control to kernel */
            elf_return_to_kernel();
            return 0;
        }

        case SYSCALL_FILE_OPEN: {
            /* arg1 = pointer to filename */
            const char *filename = (const char *)arg1;
            if (!filename) {
                return (uint32_t)-1;
            }

            /* Close any previously open file */
            if (current_file) {
                fat32_close(current_file);
                current_file = NULL;
            }

            /* Convert filename to uppercase for FAT32 */
            char uppercase_filename[256];
            uint32_t i;
            for (i = 0; filename[i] && i < sizeof(uppercase_filename) - 1; i++) {
                char ch = filename[i];
                if (ch >= 'a' && ch <= 'z') {
                    uppercase_filename[i] = ch - 32;  /* Convert to uppercase */
                } else {
                    uppercase_filename[i] = ch;
                }
            }
            uppercase_filename[i] = '\0';

            /* Open the file */
            current_file = fat32_open(uppercase_filename);
            if (!current_file) {
                return (uint32_t)-1;
            }

            return 0;
        }

        case SYSCALL_FILE_READ: {
            /* arg1 = buffer pointer, arg2 = size */
            uint8_t *buffer = (uint8_t *)arg1;
            uint32_t size = arg2;

            if (!buffer || !current_file) {
                return (uint32_t)-1;
            }

            /* Read from file */
            int bytes_read = fat32_read(current_file, buffer, size);
            if (bytes_read < 0) {
                return (uint32_t)-1;
            }

            return (uint32_t)bytes_read;
        }

        case SYSCALL_FILE_CLOSE: {
            if (!current_file) {
                return (uint32_t)-1;
            }

            fat32_close(current_file);
            current_file = NULL;
            return 0;
        }

        case SYSCALL_LIST_DIR: {
            /* arg1 = buffer pointer, arg2 = max entries */
            fat32_dirinfo_t *buffer = (fat32_dirinfo_t *)arg1;
            uint32_t max_entries = arg2;

            if (!buffer || max_entries == 0) {
                return (uint32_t)-1;
            }

            /* Get directory listing */
            int count = fat32_list_dir(buffer, (int)max_entries);
            if (count < 0) {
                return (uint32_t)-1;
            }

            return (uint32_t)count;
        }

        case SYSCALL_GET_ARGS: {
            /* arg1 = argument index, arg2 = buffer pointer, arg3 = buffer size */
            int arg_idx = (int)arg1;
            char *buffer = (char *)arg2;
            uint32_t buf_size = arg3;

            /* If arg_idx == -1, return argument count */
            if (arg_idx == -1) {
                return (uint32_t)current_program_args.count;
            }

            if (!buffer || buf_size == 0) {
                return (uint32_t)-1;
            }

            /* Check if index is valid */
            if (arg_idx < 0 || arg_idx >= current_program_args.count) {
                return (uint32_t)-1;
            }

            /* Copy argument to buffer */
            uint32_t i;
            for (i = 0; i < buf_size - 1 && current_program_args.args[arg_idx][i]; i++) {
                buffer[i] = current_program_args.args[arg_idx][i];
            }
            buffer[i] = '\0';

            return 0;
        }

        case SYSCALL_GETCHAR: {
            /* Read a character from keyboard or serial */
            while (1) {
                char c = keyboard_getchar();

                /* Check serial if no keyboard input */
                if (c == 0 && serial_received(SERIAL_COM1)) {
                    c = serial_getchar(SERIAL_COM1);
                    if (c == '\r') c = '\n';
                }

                if (c != 0) {
                    return (uint32_t)(unsigned char)c;
                }
            }
        }

        case SYSCALL_EXEC: {
            /* arg1 = command string pointer */
            const char *cmd = (const char *)arg1;
            if (!cmd) {
                return (uint32_t)-1;
            }

            /* Parse command line */
            char program_name[64];
            parse_command_line(cmd, program_name, &current_program_args);

            /* Convert program name to uppercase for FAT32 */
            char filename[12];
            int i;
            for (i = 0; program_name[i] && i < 11; i++) {
                char ch = program_name[i];
                if (ch >= 'a' && ch <= 'z') {
                    filename[i] = ch - 32;
                } else {
                    filename[i] = ch;
                }
            }
            filename[i] = '\0';

            /* Try to open the file */
            fat32_file_t *file = fat32_open(filename);
            if (!file) {
                return (uint32_t)-1; /* File not found */
            }

            /* Binary buffer for loading */
            static uint8_t exec_buffer[65536];

            if (file->size > sizeof(exec_buffer)) {
                fat32_close(file);
                return (uint32_t)-2; /* File too large */
            }

            int bytes_read = fat32_read(file, exec_buffer, file->size);
            fat32_close(file);

            if (bytes_read <= 0) {
                return (uint32_t)-3; /* Failed to read */
            }

            /* Check if it's an ELF file */
            if (exec_buffer[0] != 0x7f ||
                exec_buffer[1] != 'E' ||
                exec_buffer[2] != 'L' ||
                exec_buffer[3] != 'F') {
                return (uint32_t)-4; /* Not an ELF binary */
            }

            /*
             * Save the current program's memory region (0x200000 - 0x300000)
             * so it can be restored after the new program exits.
             * This is necessary because all programs load to 0x200000.
             */
            static uint8_t saved_program[1048576]; /* 1MB buffer for saving caller */
            uint8_t *program_base = (uint8_t *)0x200000;
            uint32_t save_size = sizeof(saved_program);

            /* Save current program memory */
            for (uint32_t j = 0; j < save_size; j++) {
                saved_program[j] = program_base[j];
            }

            /* Execute the binary */
            if (elf_load_and_exec(exec_buffer, bytes_read) != 0) {
                /* Restore caller's memory before returning error */
                for (uint32_t j = 0; j < save_size; j++) {
                    program_base[j] = saved_program[j];
                }
                return (uint32_t)-5; /* Failed to execute */
            }

            /* Restore caller's memory after program exits */
            for (uint32_t j = 0; j < save_size; j++) {
                program_base[j] = saved_program[j];
            }

            return 0; /* Success */
        }

        default:
            vga_puts("[Unknown syscall]\n");
            return (uint32_t)-1;
    }
}

/* Initialize syscalls */
void syscall_init(void) {
    /* For now, nothing to initialize */
    /* In a real OS, we'd set up IDT entry for int 0x80 */
}
