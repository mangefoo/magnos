#include "elf.h"
#include "vga.h"
#include "syscall.h"
#include "paging.h"
#include "pmm.h"
#include "gdt.h"

/* setjmp/longjmp context buffer */
typedef struct {
    uint32_t ebx, esi, edi, ebp, esp, eip;
} exec_jmp_buf;

extern int exec_setjmp(exec_jmp_buf *buf) __attribute__((returns_twice));
extern void exec_longjmp(exec_jmp_buf *buf, int val) __attribute__((noreturn));

/* Stack of saved contexts for nested exec */
#define MAX_EXEC_DEPTH 8
static exec_jmp_buf exec_stack[MAX_EXEC_DEPTH];
static volatile int exec_depth = 0;

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

    /* Get entry point */
    uint32_t entry = elf_header->e_entry;

    vga_puts("Executing binary at ");
    vga_puthex(entry);
    vga_puts("\n");

    /* Check nesting depth */
    if (exec_depth >= MAX_EXEC_DEPTH) {
        vga_puts("exec: max nesting depth reached\n");
        return -1;
    }

    /* Mark userspace pages as user-accessible */
    for (uint32_t addr = 0x200000; addr < 0x300000; addr += PAGE_SIZE) {
        paging_map(addr, addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    }
    /* Map user stack page */
    paging_map(0x300000, 0x300000, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    /* Save context; returns 0 on save, 1 when restored by elf_return_to_kernel */
    int depth = exec_depth++;
    uint32_t saved_esp0;
    __asm__ volatile("mov %%esp, %0" : "=r"(saved_esp0));

    if (exec_setjmp(&exec_stack[depth]) != 0) {
        /* Returned from program exit — restore TSS kernel stack */
        exec_depth--;
        tss_set_kernel_stack(saved_esp0);
        return 0;
    }

    /* Set TSS.esp0 so interrupts from ring 3 use correct kernel stack position */
    uint32_t current_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(current_esp));
    tss_set_kernel_stack(current_esp);

    /* Switch to ring 3 via iret */
    __asm__ volatile(
        "mov $0x23, %%ax\n\t"     /* USER_DS = 0x20 | RPL 3 */
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "pushl $0x23\n\t"         /* SS = USER_DS */
        "pushl $0x301000\n\t"     /* ESP = top of user stack */
        "pushfl\n\t"              /* EFLAGS */
        "orl $0x200, (%%esp)\n\t" /* Ensure IF is set */
        "pushl $0x1B\n\t"         /* CS = USER_CS = 0x18 | RPL 3 */
        "pushl %%esi\n\t"         /* EIP = entry point */
        "iret"
        :
        : "S"(entry)
        : "memory", "eax"
    );

    /* Unreachable — return is via SYSCALL_EXIT → longjmp */
    exec_depth--;
    return 0;
}

/* Return from userspace to kernel (called by exit syscall) */
void elf_return_to_kernel(void) {
    if (exec_depth > 0) {
        exec_longjmp(&exec_stack[exec_depth - 1], 1);
    }
}
