#include "keyboard.h"

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Modifier key states */
static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t capslock_on = 0;

/* US keyboard layout - scancode to ASCII (lowercase) */
static const char scancode_to_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6',   /* 0x00 - 0x07 */
    '7', '8', '9', '0', '-', '=',  0,   '\t',  /* 0x08 - 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',    /* 0x10 - 0x17 */
    'o', 'p', '[', ']', '\n', 0,  'a', 's',    /* 0x18 - 0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    /* 0x20 - 0x27 */
    '\'', '`', 0,  '\\', 'z', 'x', 'c', 'v',   /* 0x28 - 0x2F */
    'b', 'n', 'm', ',', '.', '/', 0,  '*',     /* 0x30 - 0x37 */
    0,   ' ', 0,   0,   0,   0,   0,   0,      /* 0x38 - 0x3F */
    0,   0,   0,   0,   0,   0,   0,   '7',    /* 0x40 - 0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',    /* 0x48 - 0x4F */
    '2', '3', '0', '.',  0,   0,   0,   0,     /* 0x50 - 0x57 */
};

/* US keyboard layout - scancode to ASCII (uppercase/shifted) */
static const char scancode_to_ascii_shift[] = {
    0,    0,   '!', '@', '#', '$', '%', '^',   /* 0x00 - 0x07 */
    '&', '*', '(', ')', '_', '+',  0,   '\t',  /* 0x08 - 0x0F */
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',    /* 0x10 - 0x17 */
    'O', 'P', '{', '}', '\n', 0,  'A', 'S',    /* 0x18 - 0x1F */
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',    /* 0x20 - 0x27 */
    '"', '~', 0,  '|', 'Z', 'X', 'C', 'V',     /* 0x28 - 0x2F */
    'B', 'N', 'M', '<', '>', '?', 0,  '*',     /* 0x30 - 0x37 */
    0,   ' ', 0,   0,   0,   0,   0,   0,      /* 0x38 - 0x3F */
    0,   0,   0,   0,   0,   0,   0,   '7',    /* 0x40 - 0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',    /* 0x48 - 0x4F */
    '2', '3', '0', '.',  0,   0,   0,   0,     /* 0x50 - 0x57 */
};

/* Initialize keyboard */
void keyboard_init(void) {
    int timeout;

    /* Wait for keyboard controller to be ready (with timeout) */
    timeout = 10000;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) && timeout > 0) {
        timeout--;
    }

    /* Enable keyboard */
    outb(KB_CMD_PORT, 0xAE);

    /* Small delay */
    for (volatile int i = 0; i < 10000; i++);

    /* Clear any pending data (with timeout) */
    timeout = 100;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) && timeout > 0) {
        inb(KB_DATA_PORT);
        timeout--;
    }

    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    capslock_on = 0;
}

/* Check if a key is available */
int keyboard_has_key(void) {
    return (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) != 0;
}

/* Get raw scancode (non-blocking) */
uint8_t keyboard_get_scancode(void) {
    if (!keyboard_has_key()) {
        return 0;
    }
    return inb(KB_DATA_PORT);
}

/* Get ASCII character (non-blocking) */
char keyboard_getchar(void) {
    uint8_t scancode = keyboard_get_scancode();

    if (scancode == 0) {
        return 0;
    }

    /* Handle key release */
    if (scancode & KEY_RELEASE) {
        uint8_t released = scancode & ~KEY_RELEASE;

        switch (released) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                shift_pressed = 0;
                break;
            case KEY_LCTRL:
                ctrl_pressed = 0;
                break;
            case KEY_LALT:
                alt_pressed = 0;
                break;
        }
        return 0;
    }

    /* Handle key press */
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = 1;
            return 0;
        case KEY_LCTRL:
            ctrl_pressed = 1;
            return 0;
        case KEY_LALT:
            alt_pressed = 1;
            return 0;
        case KEY_CAPSLOCK:
            capslock_on = !capslock_on;
            return 0;
        case KEY_BACKSPACE:
            return '\b';
        case KEY_ESCAPE:
            return 27;  /* ESC character */
    }

    /* Convert scancode to ASCII */
    if (scancode < sizeof(scancode_to_ascii)) {
        char c;

        /* Determine if we should use shifted characters */
        int use_shift = shift_pressed;

        /* For letters, capslock inverts the shift state */
        if (scancode >= 0x10 && scancode <= 0x19) {  /* Q to P */
            use_shift = shift_pressed ^ capslock_on;
        } else if (scancode >= 0x1E && scancode <= 0x26) {  /* A to L */
            use_shift = shift_pressed ^ capslock_on;
        } else if (scancode >= 0x2C && scancode <= 0x32) {  /* Z to M */
            use_shift = shift_pressed ^ capslock_on;
        }

        if (use_shift) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        return c;
    }

    return 0;
}

/* Check if shift is pressed */
int keyboard_shift_pressed(void) {
    return shift_pressed;
}

/* Check if ctrl is pressed */
int keyboard_ctrl_pressed(void) {
    return ctrl_pressed;
}

/* Check if alt is pressed */
int keyboard_alt_pressed(void) {
    return alt_pressed;
}
