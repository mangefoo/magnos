#include "keyboard.h"
#include "io.h"

/* Modifier key states */
static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t capslock_on = 0;

/* Ring buffer for interrupt-driven keyboard input */
#define KB_BUFFER_SIZE 256
static volatile char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_head = 0;
static volatile int kb_tail = 0;

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

/* Translate a scancode to ASCII, updating modifier state.
 * Returns 0 for modifier keys and key releases. */
static char translate_scancode(uint8_t scancode) {
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
            return 27;
    }

    /* Convert scancode to ASCII */
    if (scancode < sizeof(scancode_to_ascii)) {
        int use_shift = shift_pressed;

        if (scancode >= 0x10 && scancode <= 0x19) {
            use_shift = shift_pressed ^ capslock_on;
        } else if (scancode >= 0x1E && scancode <= 0x26) {
            use_shift = shift_pressed ^ capslock_on;
        } else if (scancode >= 0x2C && scancode <= 0x32) {
            use_shift = shift_pressed ^ capslock_on;
        }

        if (use_shift) {
            return scancode_to_ascii_shift[scancode];
        } else {
            return scancode_to_ascii[scancode];
        }
    }

    return 0;
}

/* Initialize keyboard */
void keyboard_init(void) {
    int timeout;

    /* Wait for keyboard controller to be ready (with timeout) */
    timeout = 10000;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) && timeout > 0) {
        timeout--;
    }

    /* Read current controller configuration */
    outb(KB_CMD_PORT, 0x20);
    timeout = 10000;
    while (!(inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) && timeout > 0) {
        timeout--;
    }
    uint8_t config = inb(KB_DATA_PORT);

    /* Enable keyboard interrupt (bit 0) */
    config |= 0x01;

    /* Write back configuration */
    timeout = 10000;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) && timeout > 0) {
        timeout--;
    }
    outb(KB_CMD_PORT, 0x60);
    timeout = 10000;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) && timeout > 0) {
        timeout--;
    }
    outb(KB_DATA_PORT, config);

    /* Enable keyboard interface */
    timeout = 10000;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) && timeout > 0) {
        timeout--;
    }
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
    kb_head = 0;
    kb_tail = 0;
}

/* IRQ1 handler — called from ISR dispatcher */
void keyboard_irq_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);

    char c = translate_scancode(scancode);

    if (c != 0) {
        int next_head = (kb_head + 1) % KB_BUFFER_SIZE;
        if (next_head != kb_tail) {
            kb_buffer[kb_head] = c;
            kb_head = next_head;
        }
        /* If buffer full, drop the character */
    }
}

/* Get ASCII character from ring buffer (non-blocking) */
char keyboard_getchar(void) {
    if (kb_tail == kb_head) {
        return 0;
    }

    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
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
