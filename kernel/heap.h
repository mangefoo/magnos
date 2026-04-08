#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

/* Initialize the kernel heap */
void heap_init(void);

/* Allocate size bytes from the kernel heap */
void *kmalloc(uint32_t size);

/* Free a previously allocated pointer */
void kfree(void *ptr);

/* Heap statistics */
typedef struct {
    uint32_t total_bytes;
    uint32_t free_bytes;
    uint32_t used_bytes;
    uint32_t num_free_blocks;
    uint32_t num_pages;
} heap_stats_t;

void heap_get_stats(heap_stats_t *stats);

#endif /* HEAP_H */
