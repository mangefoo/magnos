#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

/* Initialize the PS/2 mouse and clamp it to the framebuffer extents. */
void mouse_init(void);

/* Called from IRQ12 dispatcher. */
void mouse_irq_handler(void);

/* Current cursor position. */
int mouse_x(void);
int mouse_y(void);

/* Bitmask of currently held buttons. */
uint8_t mouse_buttons(void);

#endif /* MOUSE_H */
