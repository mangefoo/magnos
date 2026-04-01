#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

/* Serial ports */
#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8

/* Initialize serial port */
int serial_init(uint16_t port);

/* Write character to serial port */
void serial_putchar(uint16_t port, char c);

/* Write string to serial port */
void serial_puts(uint16_t port, const char *str);

/* Read character from serial port (blocking) */
char serial_getchar(uint16_t port);

/* Check if data is available to read */
int serial_received(uint16_t port);

/* Check if transmit buffer is empty */
int serial_is_transmit_empty(uint16_t port);

#endif /* SERIAL_H */
