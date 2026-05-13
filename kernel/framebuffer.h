#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

/* Boot info struct populated by the bootloader at physical 0x9000. */
typedef struct {
    uint32_t pitch;      /* bytes per scanline */
    uint32_t width;      /* pixels */
    uint32_t height;     /* pixels */
    uint32_t bpp;        /* bits per pixel (16/24/32) */
    uint32_t lfb_phys;   /* physical address of the linear framebuffer */
    uint32_t mode_ok;    /* 1 if VBE mode was set successfully */
} boot_info_t;

#define BOOT_INFO_ADDR  0x9000

/* 32-bit ARGB-ish color helpers (alpha ignored). */
#define FB_RGB(r, g, b)  (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

#define FB_BLACK         FB_RGB(0x00, 0x00, 0x00)
#define FB_WHITE         FB_RGB(0xFF, 0xFF, 0xFF)
#define FB_LIGHT_GREY    FB_RGB(0xC0, 0xC0, 0xC0)
#define FB_GREY          FB_RGB(0x90, 0x90, 0x90)
#define FB_DARK_GREY     FB_RGB(0x40, 0x40, 0x40)
#define FB_DESKTOP       FB_RGB(0x10, 0x60, 0xA0)
#define FB_TITLEBAR      FB_RGB(0x20, 0x40, 0x80)
#define FB_BUTTON_FACE   FB_RGB(0xD8, 0xD8, 0xD8)
#define FB_BUTTON_HIGH   FB_RGB(0xFF, 0xFF, 0xFF)
#define FB_BUTTON_SHADOW FB_RGB(0x60, 0x60, 0x60)
#define FB_DISPLAY_BG    FB_RGB(0x10, 0x10, 0x10)
#define FB_DISPLAY_FG    FB_RGB(0x30, 0xFF, 0x60)
#define FB_RED           FB_RGB(0xCC, 0x30, 0x30)
#define FB_ORANGE        FB_RGB(0xE8, 0x90, 0x20)

/* Initialize the framebuffer from the boot info struct. Returns 0 on success. */
int fb_init(void);

/* Has fb_init succeeded? */
int fb_is_ready(void);

/* Geometry. */
int fb_width(void);
int fb_height(void);

/* Draw primitives — all clip to screen. */
void fb_clear(uint32_t color);
void fb_pixel(int x, int y, uint32_t color);
void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_rect(int x, int y, int w, int h, uint32_t color);
void fb_hline(int x, int y, int w, uint32_t color);
void fb_vline(int x, int y, int h, uint32_t color);

/* Save / restore a small block of pixels (for the software cursor). */
void fb_save_block(int x, int y, int w, int h, uint32_t *out);
void fb_restore_block(int x, int y, int w, int h, const uint32_t *in);

/* Draw a single 8x8 glyph at (x,y) with scale `scale` (1, 2, 3, ...). */
void fb_draw_char(int x, int y, char c, uint32_t fg, int scale);

/* Draw a null-terminated string. */
void fb_draw_string(int x, int y, const char *s, uint32_t fg, int scale);

/* Width of a string rendered with `scale`. */
int fb_text_width(const char *s, int scale);

#endif /* FRAMEBUFFER_H */
