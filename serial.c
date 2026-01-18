#include "serial.h"

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Serial port register offsets */
#define SERIAL_DATA         0   /* Data register (read/write) */
#define SERIAL_INT_ENABLE   1   /* Interrupt enable register */
#define SERIAL_FIFO_CTRL    2   /* FIFO control register */
#define SERIAL_LINE_CTRL    3   /* Line control register */
#define SERIAL_MODEM_CTRL   4   /* Modem control register */
#define SERIAL_LINE_STATUS  5   /* Line status register */
#define SERIAL_MODEM_STATUS 6   /* Modem status register */
#define SERIAL_SCRATCH      7   /* Scratch register */

/* Initialize serial port */
int serial_init(uint16_t port) {
    /* Disable interrupts */
    outb(port + SERIAL_INT_ENABLE, 0x00);

    /* Enable DLAB (set baud rate divisor) */
    outb(port + SERIAL_LINE_CTRL, 0x80);

    /* Set divisor to 3 (38400 baud) */
    outb(port + SERIAL_DATA, 0x03);
    outb(port + SERIAL_INT_ENABLE, 0x00);

    /* 8 bits, no parity, one stop bit */
    outb(port + SERIAL_LINE_CTRL, 0x03);

    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(port + SERIAL_FIFO_CTRL, 0xC7);

    /* IRQs enabled, RTS/DSR set */
    outb(port + SERIAL_MODEM_CTRL, 0x0B);

    /* Test serial chip (loopback mode) */
    outb(port + SERIAL_MODEM_CTRL, 0x1E);
    outb(port + SERIAL_DATA, 0xAE);

    if (inb(port + SERIAL_DATA) != 0xAE) {
        return 1;  /* Faulty serial port */
    }

    /* Set to normal operation mode */
    outb(port + SERIAL_MODEM_CTRL, 0x0F);

    return 0;
}

/* Check if transmit buffer is empty */
int serial_is_transmit_empty(uint16_t port) {
    return inb(port + SERIAL_LINE_STATUS) & 0x20;
}

/* Write character to serial port */
void serial_putchar(uint16_t port, char c) {
    /* Wait for transmit buffer to be empty */
    while (serial_is_transmit_empty(port) == 0)
        ;

    outb(port + SERIAL_DATA, c);
}

/* Write string to serial port */
void serial_puts(uint16_t port, const char *str) {
    while (*str) {
        if (*str == '\n') {
            serial_putchar(port, '\r');
        }
        serial_putchar(port, *str++);
    }
}

/* Check if data is available to read */
int serial_received(uint16_t port) {
    return inb(port + SERIAL_LINE_STATUS) & 1;
}

/* Read character from serial port */
char serial_getchar(uint16_t port) {
    /* Wait for data */
    while (serial_received(port) == 0)
        ;

    return inb(port + SERIAL_DATA);
}
