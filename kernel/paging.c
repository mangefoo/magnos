#include "paging.h"
#include "pmm.h"

/* Number of page directory entries to identity-map (4 = 16MB) */
#define IDENTITY_MAP_ENTRIES 4

uint32_t *kernel_page_directory = 0;

/* Zero a 4KB page */
static void zero_page(void *page) {
    uint32_t *p = (uint32_t *)page;
    for (int i = 0; i < 1024; i++) {
        p[i] = 0;
    }
}

void paging_init(void) {
    /* Allocate and zero the page directory */
    kernel_page_directory = (uint32_t *)pmm_alloc();
    zero_page(kernel_page_directory);

    /* Identity-map 0-16MB (4 page directory entries, 4 page tables) */
    for (int i = 0; i < IDENTITY_MAP_ENTRIES; i++) {
        uint32_t *pt = (uint32_t *)pmm_alloc();
        zero_page(pt);

        /* Fill page table: each entry maps to its corresponding physical page */
        for (int j = 0; j < 1024; j++) {
            uint32_t phys = (i * 1024 + j) * PAGE_SIZE;
            pt[j] = phys | PAGE_PRESENT | PAGE_WRITABLE;
        }

        kernel_page_directory[i] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* Load page directory into CR3 */
    __asm__ volatile("mov %0, %%cr3" : : "r"(kernel_page_directory));

    /* Enable paging: set PG bit (bit 31) in CR0 */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    /* Allocate page table if not present */
    if (!(kernel_page_directory[pd_idx] & PAGE_PRESENT)) {
        uint32_t *pt = (uint32_t *)pmm_alloc();
        if (!pt) return;
        zero_page(pt);
        kernel_page_directory[pd_idx] = (uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE;
    }

    uint32_t *pt = (uint32_t *)PAGE_FRAME(kernel_page_directory[pd_idx]);
    pt[pt_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;

    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void paging_unmap(uint32_t virt) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    if (!(kernel_page_directory[pd_idx] & PAGE_PRESENT))
        return;

    uint32_t *pt = (uint32_t *)PAGE_FRAME(kernel_page_directory[pd_idx]);
    pt[pt_idx] = 0;

    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

uint32_t paging_get_phys(uint32_t virt) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    if (!(kernel_page_directory[pd_idx] & PAGE_PRESENT))
        return 0;

    uint32_t *pt = (uint32_t *)PAGE_FRAME(kernel_page_directory[pd_idx]);
    if (!(pt[pt_idx] & PAGE_PRESENT))
        return 0;

    return PAGE_FRAME(pt[pt_idx]) | (virt & 0xFFF);
}
