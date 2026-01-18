#include "elf.h"
#include "vga.h"
#include "syscall.h"

/* Saved context for returning from userspace */
static uint32_t saved_esp;
static uint32_t saved_ebp;
static uint32_t saved_eip;
static volatile int binary_exited;

/* Memory functions */
static void* memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

/* Validate ELF header */
int elf_validate_header(elf32_ehdr_t *header) {
    /* Check magic number */
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3) {
        return -1;
    }

    /* Check class (32-bit) */
    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        return -1;
    }

    /* Check endianness (little-endian) */
    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return -1;
    }

    /* Check machine type (x86) */
    if (header->e_machine != EM_386) {
        return -1;
    }

    /* Check type (executable or position-independent) */
    if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
        return -1;
    }

    return 0;
}

/* Load and execute an ELF binary */
int elf_load_and_exec(uint8_t *elf_data, uint32_t size) {
    /* Parse ELF header */
    elf32_ehdr_t *elf_header = (elf32_ehdr_t *)elf_data;

    /* Validate header */
    if (elf_validate_header(elf_header) != 0) {
        vga_puts("Invalid ELF header\n");
        return -1;
    }

    /* Get program headers */
    elf32_phdr_t *ph = (elf32_phdr_t *)(elf_data + elf_header->e_phoff);

    /* Load all LOAD segments */
    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {
            /* Calculate load address */
            uint8_t *dest = (uint8_t *)ph[i].p_vaddr;

            /* Copy segment data */
            if (ph[i].p_filesz > 0) {
                memcpy(dest, elf_data + ph[i].p_offset, ph[i].p_filesz);
            }

            /* Zero out remaining memory (BSS) */
            if (ph[i].p_memsz > ph[i].p_filesz) {
                memset(dest + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);
            }
        }
    }

    /* Set up syscall handler pointer at fixed address for userspace */
    uint32_t *syscall_ptr = (uint32_t *)0x00100000;
    *syscall_ptr = (uint32_t)syscall_handler;

    /* Get entry point */
    uint32_t entry = elf_header->e_entry;

    vga_puts("Executing binary at ");
    vga_puthex(entry);
    vga_puts("\n");

    /* Reset exit flag */
    binary_exited = 0;

    /* Save current stack state */
    __asm__ volatile(
        "mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        : "=m"(saved_esp), "=m"(saved_ebp)
    );

    /* Create function pointer and call it */
    void (*entry_func)(void) = (void (*)(void))entry;
    entry_func();

    /* We reach here either normally or after elf_return_to_kernel restores stack */
    return 0;
}

/* Return from userspace to kernel (called by exit syscall) */
void elf_return_to_kernel(void) {
    /* Restore stack and return to elf_load_and_exec */
    __asm__ volatile(
        "mov %0, %%esp\n"
        "mov %1, %%ebp\n"
        "xor %%eax, %%eax\n"  /* Return 0 */
        "leave\n"
        "ret\n"
        :
        : "m"(saved_esp), "m"(saved_ebp)
    );
}
