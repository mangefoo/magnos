#ifndef GUI_H
#define GUI_H

#include <stdint.h>

/* Bevel direction for the 3D border. */
typedef enum {
    GUI_BEVEL_RAISED,
    GUI_BEVEL_SUNKEN,
} gui_bevel_t;

/* Draw the desktop background (solid colour fill). */
void gui_draw_desktop(void);

/* Draw a window frame with a title bar. */
void gui_draw_window(int x, int y, int w, int h, const char *title);

/* Draw a beveled panel (no text). */
void gui_draw_bevel(int x, int y, int w, int h, gui_bevel_t style);

/* Draw a button. `pressed` shifts the label and inverts the bevel. */
void gui_draw_button(int x, int y, int w, int h,
                     const char *label, int pressed, int hovered);

/* Returns 1 if (mx,my) lies inside (x,y,w,h). */
int gui_hit(int x, int y, int w, int h, int mx, int my);

/* Mouse cursor — saves a backing block then draws a small arrow. */
void gui_cursor_show(int x, int y);
void gui_cursor_hide(void);

/* Initialize internal state (call once after framebuffer is up). */
void gui_init(void);

#endif /* GUI_H */
