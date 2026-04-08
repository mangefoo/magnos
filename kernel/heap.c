#include "heap.h"
#include "pmm.h"

#define HEADER_SIZE     16      /* sizeof(block_header_t), padded */
#define MIN_ALLOC       16      /* Minimum allocation size */
#define ALIGN           16      /* Alignment for all allocations */
#define INITIAL_PAGES   16      /* 64KB initial heap */

/* Block header — placed at the start of every block */
typedef struct block_header {
    uint32_t size;              /* Usable size (excludes header) */
    uint32_t is_free;
    struct block_header *next;  /* Next block in address-sorted list */
    uint32_t _pad;              /* Pad to 16 bytes */
} block_header_t;

static block_header_t *free_list = (void *)0;
static uint32_t heap_pages = 0;

/* Insert a block into the free list in address-sorted order */
static void list_insert(block_header_t *block) {
    block->is_free = 1;

    if (!free_list || (uint32_t)block < (uint32_t)free_list) {
        block->next = free_list;
        free_list = block;
        return;
    }

    block_header_t *cur = free_list;
    while (cur->next && (uint32_t)cur->next < (uint32_t)block) {
        cur = cur->next;
    }
    block->next = cur->next;
    cur->next = block;
}

/* Try to coalesce a block with its next neighbor */
static void coalesce_forward(block_header_t *block) {
    if (!block->next)
        return;
    /* Check if next block is physically adjacent */
    uint8_t *end = (uint8_t *)block + HEADER_SIZE + block->size;
    if (end == (uint8_t *)block->next && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }
}

/* Add a new page from PMM to the heap */
static block_header_t *heap_add_page(void) {
    uint32_t page = pmm_alloc();
    if (page == 0)
        return (void *)0;

    block_header_t *block = (block_header_t *)page;
    block->size = PAGE_SIZE - HEADER_SIZE;
    block->is_free = 1;
    block->next = (void *)0;
    heap_pages++;

    list_insert(block);

    /* Coalesce with neighbors */
    block_header_t *cur = free_list;
    while (cur) {
        coalesce_forward(cur);
        cur = cur->next;
    }

    return block;
}

void heap_init(void) {
    for (int i = 0; i < INITIAL_PAGES; i++) {
        heap_add_page();
    }
}

void *kmalloc(uint32_t size) {
    if (size == 0)
        return (void *)0;

    /* Round up to alignment */
    size = (size + ALIGN - 1) & ~(ALIGN - 1);
    if (size < MIN_ALLOC)
        size = MIN_ALLOC;

    /* First-fit search */
    block_header_t *cur = free_list;
    block_header_t *prev = (void *)0;

    while (cur) {
        if (cur->is_free && cur->size >= size) {
            /* Can we split? */
            if (cur->size >= size + HEADER_SIZE + MIN_ALLOC) {
                block_header_t *new_block = (block_header_t *)((uint8_t *)cur + HEADER_SIZE + size);
                new_block->size = cur->size - size - HEADER_SIZE;
                new_block->is_free = 1;
                new_block->next = cur->next;
                cur->next = new_block;
                cur->size = size;
            }

            /* Remove from free list */
            cur->is_free = 0;
            if (prev)
                prev->next = cur->next;
            else
                free_list = cur->next;
            cur->next = (void *)0;

            return (void *)((uint8_t *)cur + HEADER_SIZE);
        }
        prev = cur;
        cur = cur->next;
    }

    /* No block found — try to grow the heap */
    uint32_t pages_needed = (size + HEADER_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < pages_needed; i++) {
        if (!heap_add_page())
            return (void *)0;
    }

    /* Retry */
    cur = free_list;
    prev = (void *)0;
    while (cur) {
        if (cur->is_free && cur->size >= size) {
            if (cur->size >= size + HEADER_SIZE + MIN_ALLOC) {
                block_header_t *new_block = (block_header_t *)((uint8_t *)cur + HEADER_SIZE + size);
                new_block->size = cur->size - size - HEADER_SIZE;
                new_block->is_free = 1;
                new_block->next = cur->next;
                cur->next = new_block;
                cur->size = size;
            }

            cur->is_free = 0;
            if (prev)
                prev->next = cur->next;
            else
                free_list = cur->next;
            cur->next = (void *)0;

            return (void *)((uint8_t *)cur + HEADER_SIZE);
        }
        prev = cur;
        cur = cur->next;
    }

    return (void *)0;
}

void kfree(void *ptr) {
    if (!ptr)
        return;

    block_header_t *block = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);

    /* Insert back into free list (address-sorted) */
    list_insert(block);

    /* Coalesce all adjacent free blocks */
    block_header_t *cur = free_list;
    while (cur) {
        coalesce_forward(cur);
        cur = cur->next;
    }
}

void heap_get_stats(heap_stats_t *stats) {
    stats->total_bytes = heap_pages * PAGE_SIZE;
    stats->free_bytes = 0;
    stats->num_free_blocks = 0;
    stats->num_pages = heap_pages;

    block_header_t *cur = free_list;
    while (cur) {
        if (cur->is_free) {
            stats->free_bytes += cur->size;
            stats->num_free_blocks++;
        }
        cur = cur->next;
    }
    stats->used_bytes = stats->total_bytes - stats->free_bytes;
}
