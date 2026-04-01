#include "vga.h"

static uint16_t *vga_buffer;
static size_t vga_row;
static size_t vga_column;
static uint8_t vga_color;

/* Create VGA color attribute */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

/* Create VGA entry with character and color */
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Port I/O */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Update hardware cursor to match software position */
static void vga_update_cursor(void) {
    uint16_t pos = vga_row * VGA_WIDTH + vga_column;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* Initialize VGA */
void vga_init(void) {
    vga_buffer = (uint16_t *)VGA_MEMORY;
    vga_row = 0;
    vga_column = 0;
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

/* Clear screen */
void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_column = 0;
    vga_update_cursor();
}

/* Scroll screen up by one line */
static void vga_scroll(void) {
    /* Move all lines up by one */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src = (y + 1) * VGA_WIDTH + x;
            const size_t dst = y * VGA_WIDTH + x;
            vga_buffer[dst] = vga_buffer[src];
        }
    }

    /* Clear the last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = vga_entry(' ', vga_color);
    }

    vga_row = VGA_HEIGHT - 1;
}

/* Put character at current position */
void vga_putchar(char c) {
    /* Handle special characters */
    if (c == '\n') {
        vga_column = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_scroll();
        }
    } else if (c == '\r') {
        vga_column = 0;
    } else if (c == '\b') {
        if (vga_column > 0) {
            vga_column--;
        } else if (vga_row > 0) {
            vga_row--;
            vga_column = VGA_WIDTH - 1;
        }
    } else if (c == '\t') {
        vga_column = (vga_column + 4) & ~3;
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            if (++vga_row >= VGA_HEIGHT) {
                vga_scroll();
            }
        }
    } else {
        /* Put character on screen */
        const size_t index = vga_row * VGA_WIDTH + vga_column;
        vga_buffer[index] = vga_entry(c, vga_color);

        /* Advance cursor */
        if (++vga_column >= VGA_WIDTH) {
            vga_column = 0;
            if (++vga_row >= VGA_HEIGHT) {
                vga_scroll();
            }
        }
    }

    vga_update_cursor();
}

/* Write string */
void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/* Set colors */
void vga_set_color(enum vga_color fg, enum vga_color bg) {
    vga_color = vga_entry_color(fg, bg);
}

/* Write hex number */
void vga_puthex(uint32_t n) {
    const char *digits = "0123456789ABCDEF";
    vga_puts("0x");

    for (int i = 28; i >= 0; i -= 4) {
        vga_putchar(digits[(n >> i) & 0xF]);
    }
}
