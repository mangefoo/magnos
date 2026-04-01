#ifndef IO_H
#define IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Short delay for PIC/hardware timing (~1μs) */
static inline void io_wait(void) {
    outb(0x80, 0);
}

/* Bulk 16-bit port reads (for IDE) */
static inline void inw_buffer(uint16_t port, uint16_t *buffer, uint32_t count) {
    __asm__ volatile ("cld; rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

/* Bulk 16-bit port writes (for IDE) */
static inline void outw_buffer(uint16_t port, uint16_t *buffer, uint32_t count) {
    __asm__ volatile ("cld; rep outsw" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

#endif /* IO_H */
