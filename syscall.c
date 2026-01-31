#include "syscall.h"
#include "vga.h"
#include "elf.h"
#include "fat32.h"
#include "serial.h"
#include "args.h"

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
                vga_puts(str);
                serial_puts(SERIAL_COM1, str);
            }
            return 0;
        }

        case SYSCALL_EXIT: {
            /* arg1 = exit code */
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

            /* Open the file */
            current_file = fat32_open(filename);
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
