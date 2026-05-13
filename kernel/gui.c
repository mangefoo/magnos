#include "gui.h"
#include "framebuffer.h"

#define CURSOR_W 12
#define CURSOR_H 19

/* Backing store for the area under the cursor and the position it was drawn at. */
static uint32_t cursor_backing[CURSOR_W * CURSOR_H];
static int cursor_saved = 0;
static int cursor_sx = 0;
static int cursor_sy = 0;

/* Arrow cursor — 1 = white fill, 2 = black outline, 0 = transparent.
 * 12 wide, 19 tall. */
static const uint8_t cursor_pixels[CURSOR_H][CURSOR_W] = {
    {2,0,0,0,0,0,0,0,0,0,0,0},
    {2,2,0,0,0,0,0,0,0,0,0,0},
    {2,1,2,0,0,0,0,0,0,0,0,0},
    {2,1,1,2,0,0,0,0,0,0,0,0},
    {2,1,1,1,2,0,0,0,0,0,0,0},
    {2,1,1,1,1,2,0,0,0,0,0,0},
    {2,1,1,1,1,1,2,0,0,0,0,0},
    {2,1,1,1,1,1,1,2,0,0,0,0},
    {2,1,1,1,1,1,1,1,2,0,0,0},
    {2,1,1,1,1,1,1,1,1,2,0,0},
    {2,1,1,1,1,1,1,1,1,1,2,0},
    {2,1,1,1,1,1,1,2,2,2,2,2},
    {2,1,1,2,1,1,2,0,0,0,0,0},
    {2,1,2,2,1,1,2,0,0,0,0,0},
    {2,2,0,0,2,1,1,2,0,0,0,0},
    {0,0,0,0,2,1,1,2,0,0,0,0},
    {0,0,0,0,0,2,1,1,2,0,0,0},
    {0,0,0,0,0,2,1,1,2,0,0,0},
    {0,0,0,0,0,0,2,2,0,0,0,0},
};

void gui_init(void) {
    cursor_saved = 0;
}

void gui_draw_desktop(void) {
    fb_clear(FB_DESKTOP);
    /* Title strip across the top */
    fb_fill_rect(0, 0, fb_width(), 24, FB_TITLEBAR);
    fb_draw_string(8, 8, "MagnOS Desktop", FB_WHITE, 1);
}

void gui_draw_bevel(int x, int y, int w, int h, gui_bevel_t style) {
    uint32_t hi = (style == GUI_BEVEL_RAISED) ? FB_BUTTON_HIGH : FB_BUTTON_SHADOW;
    uint32_t lo = (style == GUI_BEVEL_RAISED) ? FB_BUTTON_SHADOW : FB_BUTTON_HIGH;

    fb_hline(x, y, w, hi);
    fb_vline(x, y, h, hi);
    fb_hline(x, y + h - 1, w, lo);
    fb_vline(x + w - 1, y, h, lo);
}

void gui_draw_window(int x, int y, int w, int h, const char *title) {
    /* Outer raised frame */
    fb_fill_rect(x, y, w, h, FB_LIGHT_GREY);
    gui_draw_bevel(x, y, w, h, GUI_BEVEL_RAISED);
    gui_draw_bevel(x + 1, y + 1, w - 2, h - 2, GUI_BEVEL_RAISED);

    /* Title bar */
    int tb_h = 20;
    fb_fill_rect(x + 4, y + 4, w - 8, tb_h, FB_TITLEBAR);
    fb_draw_string(x + 10, y + 4 + 6, title, FB_WHITE, 1);
}

void gui_draw_button(int x, int y, int w, int h,
                     const char *label, int pressed, int hovered) {
    uint32_t face = hovered ? FB_BUTTON_HIGH : FB_BUTTON_FACE;
    fb_fill_rect(x, y, w, h, face);
    gui_draw_bevel(x, y, w, h, pressed ? GUI_BEVEL_SUNKEN : GUI_BEVEL_RAISED);
    gui_draw_bevel(x + 1, y + 1, w - 2, h - 2, pressed ? GUI_BEVEL_SUNKEN : GUI_BEVEL_RAISED);

    /* Center the label in the button */
    int scale = 2;
    int tw = fb_text_width(label, scale);
    int th = 8 * scale;
    int tx = x + (w - tw) / 2 + (pressed ? 1 : 0);
    int ty = y + (h - th) / 2 + (pressed ? 1 : 0);
    fb_draw_string(tx, ty, label, FB_BLACK, scale);
}

int gui_hit(int x, int y, int w, int h, int mx, int my) {
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

void gui_cursor_show(int x, int y) {
    int W = fb_width();
    int H = fb_height();
    int cw = CURSOR_W, ch = CURSOR_H;
    if (x + cw > W) cw = W - x;
    if (y + ch > H) ch = H - y;
    if (cw <= 0 || ch <= 0) return;

    /* Save what's under the cursor at the new position */
    fb_save_block(x, y, cw, ch, cursor_backing);
    cursor_sx = x;
    cursor_sy = y;
    cursor_saved = 1;

    /* Paint cursor */
    for (int row = 0; row < ch; row++) {
        for (int col = 0; col < cw; col++) {
            uint8_t v = cursor_pixels[row][col];
            if (v == 1) fb_pixel(x + col, y + row, FB_WHITE);
            else if (v == 2) fb_pixel(x + col, y + row, FB_BLACK);
        }
    }
}

void gui_cursor_hide(void) {
    if (!cursor_saved) return;
    int W = fb_width();
    int H = fb_height();
    int cw = CURSOR_W, ch = CURSOR_H;
    if (cursor_sx + cw > W) cw = W - cursor_sx;
    if (cursor_sy + ch > H) ch = H - cursor_sy;
    if (cw > 0 && ch > 0) {
        fb_restore_block(cursor_sx, cursor_sy, cw, ch, cursor_backing);
    }
    cursor_saved = 0;
}
