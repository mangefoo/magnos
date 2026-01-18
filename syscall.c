#include "syscall.h"
#include "vga.h"
#include "elf.h"

/* Memory functions */
static uint32_t strlen(const char *str) {
    uint32_t len = 0;
    while (str[len]) len++;
    return len;
}

/* Syscall handler */
void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    (void)arg2;  /* Unused */
    (void)arg3;  /* Unused */

    switch (syscall_num) {
        case SYSCALL_PRINT: {
            /* arg1 = pointer to string */
            const char *str = (const char *)arg1;
            if (str) {
                vga_puts(str);
            }
            break;
        }

        case SYSCALL_EXIT: {
            /* arg1 = exit code */
            vga_puts("\n[Program exited with code: ");
            vga_puthex(arg1);
            vga_puts("]\n");
            /* Return control to kernel */
            elf_return_to_kernel();
            break;
        }

        default:
            vga_puts("[Unknown syscall]\n");
            break;
    }
}

/* Initialize syscalls */
void syscall_init(void) {
    /* For now, nothing to initialize */
    /* In a real OS, we'd set up IDT entry for int 0x80 */
}
