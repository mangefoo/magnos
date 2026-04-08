#include "pmm.h"

/* Bitmap: 1 bit per page. 1 = used, 0 = free. */
static uint8_t bitmap[TOTAL_PAGES / 8];

#define BITMAP_SET(p)    (bitmap[(p) / 8] |=  (1 << ((p) % 8)))
#define BITMAP_CLEAR(p)  (bitmap[(p) / 8] &= ~(1 << ((p) % 8)))
#define BITMAP_TEST(p)   (bitmap[(p) / 8] &   (1 << ((p) % 8)))

void pmm_init(void) {
    extern uint32_t _kernel_end;

    /* Mark all pages as used */
    for (uint32_t i = 0; i < sizeof(bitmap); i++) {
        bitmap[i] = 0xFF;
    }

    /* Calculate kernel end page (rounded up) */
    uint32_t kend = (uint32_t)&_kernel_end;
    uint32_t kend_page = (kend + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Free pages between kernel end and kernel stack (0x1F0000),
     * but skip the BIOS/VGA/ROM area (0xA0000-0xFFFFF) */
    for (uint32_t p = kend_page; p < 0x1F0000 / PAGE_SIZE; p++) {
        if (p >= 0xA0000 / PAGE_SIZE && p <= 0xFFFFF / PAGE_SIZE)
            continue;
        BITMAP_CLEAR(p);
    }

    /* Free pages above the userspace area (0x300000+) */
    for (uint32_t p = 0x300000 / PAGE_SIZE; p < TOTAL_PAGES; p++) {
        BITMAP_CLEAR(p);
    }
}

uint32_t pmm_alloc(void) {
    for (uint32_t i = 0; i < sizeof(bitmap); i++) {
        if (bitmap[i] == 0xFF)
            continue;
        for (int bit = 0; bit < 8; bit++) {
            if (!(bitmap[i] & (1 << bit))) {
                bitmap[i] |= (1 << bit);
                return (i * 8 + bit) * PAGE_SIZE;
            }
        }
    }
    return 0;  /* Out of memory */
}

void pmm_free(uint32_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    if (page > 0 && page < TOTAL_PAGES) {
        BITMAP_CLEAR(page);
    }
}

uint32_t pmm_get_free_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < sizeof(bitmap); i++) {
        uint8_t b = bitmap[i];
        /* Count zero bits (free pages) */
        for (int bit = 0; bit < 8; bit++) {
            if (!(b & (1 << bit)))
                count++;
        }
    }
    return count;
}

uint32_t pmm_get_total_count(void) {
    return TOTAL_PAGES;
}
