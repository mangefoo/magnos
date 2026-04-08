#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* Page table entry flags */
#define PAGE_PRESENT    0x01
#define PAGE_WRITABLE   0x02
#define PAGE_USER       0x04

/* Extract page directory / page table indices from a virtual address */
#define PD_INDEX(virt)    (((virt) >> 22) & 0x3FF)
#define PT_INDEX(virt)    (((virt) >> 12) & 0x3FF)
#define PAGE_FRAME(entry) ((entry) & 0xFFFFF000)

/* Kernel page directory (physical = virtual under identity mapping) */
extern uint32_t *kernel_page_directory;

/* Initialize paging with identity mapping for 0-16MB */
void paging_init(void);

/* Map a virtual page to a physical page with given flags */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);

/* Unmap a virtual page */
void paging_unmap(uint32_t virt);

/* Get the physical address mapped to a virtual address (0 if unmapped) */
uint32_t paging_get_phys(uint32_t virt);

#endif /* PAGING_H */
