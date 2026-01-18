#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/* ELF-32 Header */
#define EI_NIDENT 16

typedef struct {
    uint8_t  e_ident[EI_NIDENT];  /* Magic number and other info */
    uint16_t e_type;               /* Object file type */
    uint16_t e_machine;            /* Architecture */
    uint32_t e_version;            /* Object file version */
    uint32_t e_entry;              /* Entry point virtual address */
    uint32_t e_phoff;              /* Program header table offset */
    uint32_t e_shoff;              /* Section header table offset */
    uint32_t e_flags;              /* Processor-specific flags */
    uint16_t e_ehsize;             /* ELF header size */
    uint16_t e_phentsize;          /* Program header entry size */
    uint16_t e_phnum;              /* Number of program headers */
    uint16_t e_shentsize;          /* Section header entry size */
    uint16_t e_shnum;              /* Number of section headers */
    uint16_t e_shstrndx;           /* Section header string table index */
} __attribute__((packed)) elf32_ehdr_t;

/* ELF-32 Program Header */
typedef struct {
    uint32_t p_type;               /* Segment type */
    uint32_t p_offset;             /* Segment file offset */
    uint32_t p_vaddr;              /* Segment virtual address */
    uint32_t p_paddr;              /* Segment physical address */
    uint32_t p_filesz;             /* Segment size in file */
    uint32_t p_memsz;              /* Segment size in memory */
    uint32_t p_flags;              /* Segment flags */
    uint32_t p_align;              /* Segment alignment */
} __attribute__((packed)) elf32_phdr_t;

/* ELF identification */
#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5

#define ELFMAG0    0x7f
#define ELFMAG1    'E'
#define ELFMAG2    'L'
#define ELFMAG3    'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1

/* e_type */
#define ET_NONE    0
#define ET_REL     1
#define ET_EXEC    2
#define ET_DYN     3

/* e_machine */
#define EM_386     3

/* Program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4

/* Program header flags */
#define PF_X       0x1  /* Execute */
#define PF_W       0x2  /* Write */
#define PF_R       0x4  /* Read */

/* Load and execute an ELF binary from memory */
int elf_load_and_exec(uint8_t *elf_data, uint32_t size);

/* Validate ELF header */
int elf_validate_header(elf32_ehdr_t *header);

#endif /* ELF_H */
