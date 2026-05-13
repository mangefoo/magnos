#include "mouse.h"
#include "io.h"
#include "framebuffer.h"

/* PS/2 controller ports */
#define PS2_DATA   0x60
#define PS2_STATUS 0x64
#define PS2_CMD    0x64

/* Status register bits */
#define PS2_STATUS_OUT_FULL  0x01
#define PS2_STATUS_IN_FULL   0x02

/* Controller commands */
#define CMD_DISABLE_AUX      0xA7
#define CMD_ENABLE_AUX       0xA8
#define CMD_READ_CFG         0x20
#define CMD_WRITE_CFG        0x60
#define CMD_WRITE_AUX        0xD4

/* Mouse commands (sent through the AUX channel) */
#define MOUSE_SET_DEFAULTS   0xF6
#define MOUSE_ENABLE_REPORT  0xF4
#define MOUSE_ACK            0xFA

static volatile int cursor_x = 0;
static volatile int cursor_y = 0;
static volatile uint8_t buttons = 0;

/* Packet decode state */
static uint8_t packet[3];
static int packet_idx = 0;

static void ps2_wait_input_clear(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS) & PS2_STATUS_IN_FULL)) return;
    }
}

static void ps2_wait_output_full(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS) & PS2_STATUS_OUT_FULL) return;
    }
}

static void mouse_write(uint8_t value) {
    ps2_wait_input_clear();
    outb(PS2_CMD, CMD_WRITE_AUX);
    ps2_wait_input_clear();
    outb(PS2_DATA, value);
}

static uint8_t mouse_read(void) {
    ps2_wait_output_full();
    return inb(PS2_DATA);
}

void mouse_init(void) {
    /* Center the cursor on screen if framebuffer is up */
    if (fb_is_ready()) {
        cursor_x = fb_width() / 2;
        cursor_y = fb_height() / 2;
    }

    /* Enable the auxiliary (mouse) device */
    ps2_wait_input_clear();
    outb(PS2_CMD, CMD_ENABLE_AUX);

    /* Enable IRQ12 in the controller config byte */
    ps2_wait_input_clear();
    outb(PS2_CMD, CMD_READ_CFG);
    ps2_wait_output_full();
    uint8_t cfg = inb(PS2_DATA);
    cfg |= 0x02;   /* enable IRQ12 (aux interrupt) */
    cfg &= ~0x20;  /* clear "disable aux clock" bit */
    ps2_wait_input_clear();
    outb(PS2_CMD, CMD_WRITE_CFG);
    ps2_wait_input_clear();
    outb(PS2_DATA, cfg);

    /* Set defaults */
    mouse_write(MOUSE_SET_DEFAULTS);
    (void)mouse_read();  /* ACK */

    /* Enable streaming */
    mouse_write(MOUSE_ENABLE_REPORT);
    (void)mouse_read();  /* ACK */

    packet_idx = 0;
}

void mouse_irq_handler(void) {
    uint8_t status = inb(PS2_STATUS);
    if (!(status & PS2_STATUS_OUT_FULL)) return;
    /* Only consume bytes destined for the mouse (bit 5 of status). */
    if (!(status & 0x20)) {
        (void)inb(PS2_DATA);
        return;
    }

    uint8_t data = inb(PS2_DATA);

    /* Resync if the first packet byte doesn't look right (bit 3 = 1 always). */
    if (packet_idx == 0 && !(data & 0x08)) {
        return;
    }

    packet[packet_idx++] = data;
    if (packet_idx < 3) return;
    packet_idx = 0;

    uint8_t flags = packet[0];
    int dx = packet[1];
    int dy = packet[2];

    /* Sign-extend per the X/Y overflow flags */
    if (flags & 0x10) dx -= 256;
    if (flags & 0x20) dy -= 256;
    /* Drop packets where X/Y overflow is set */
    if (flags & 0xC0) return;

    /* PS/2 mouse Y axis is inverted relative to screen coords. */
    int nx = cursor_x + dx;
    int ny = cursor_y - dy;

    int w = fb_width();
    int h = fb_height();
    if (w == 0) w = 1024;
    if (h == 0) h = 768;

    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if (nx >= w) nx = w - 1;
    if (ny >= h) ny = h - 1;

    cursor_x = nx;
    cursor_y = ny;
    buttons = flags & 0x07;
}

int mouse_x(void) { return cursor_x; }
int mouse_y(void) { return cursor_y; }
uint8_t mouse_buttons(void) { return buttons; }
