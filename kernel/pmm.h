#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE       4096
#define TOTAL_MEMORY    (16 * 1024 * 1024)          /* 16MB assumed */
#define TOTAL_PAGES     (TOTAL_MEMORY / PAGE_SIZE)  /* 4096 pages */

/* Initialize the physical memory manager */
void pmm_init(void);

/* Allocate a single physical page, returns physical address or 0 on failure */
uint32_t pmm_alloc(void);

/* Free a previously allocated physical page */
void pmm_free(uint32_t addr);

/* Get count of free pages */
uint32_t pmm_get_free_count(void);

/* Get total number of pages */
uint32_t pmm_get_total_count(void);

#endif /* PMM_H */
