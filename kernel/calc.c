#include "calc.h"
#include "framebuffer.h"
#include "gui.h"
#include "mouse.h"

/* ----- Layout constants ----- */
#define WIN_W   320
#define WIN_H   400

#define DISPLAY_INSET    16
#define DISPLAY_H        56
#define BUTTON_GAP       8
#define BUTTON_COLS      4
#define BUTTON_ROWS      5

/* ----- Calculator state ----- */
typedef enum {
    OP_NONE = 0,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
} op_t;

static long acc        = 0;
static long current    = 0;
static op_t pending    = OP_NONE;
static int  entering   = 0;   /* are we currently typing into `current`? */
static int  has_acc    = 0;
static int  error      = 0;
static char display[24];

/* The 5x4 button grid. Empty label = no button at that cell. */
static const char *btn_grid[BUTTON_ROWS][BUTTON_COLS] = {
    { "C",  "+/-", "%",  "/" },
    { "7",  "8",   "9",  "*" },
    { "4",  "5",   "6",  "-" },
    { "1",  "2",   "3",  "+" },
    { "0",  ".",   "=",  ""  },  /* '0' spans 2 cells (handled below) */
};

static int win_x = 0;
static int win_y = 0;

/* ----- Helpers ----- */

static void itoa(long n, char *out) {
    char tmp[24];
    int i = 0, j = 0;
    int neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    if (n == 0) tmp[i++] = '0';
    while (n > 0) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }
    if (neg) tmp[i++] = '-';
    while (i > 0) out[j++] = tmp[--i];
    out[j] = '\0';
}

static void update_display(void) {
    if (error) {
        const char *s = "ERROR";
        int k = 0;
        while (s[k]) { display[k] = s[k]; k++; }
        display[k] = '\0';
        return;
    }
    long n = entering ? current : (has_acc ? acc : current);
    itoa(n, display);
}

static void apply_pending(void) {
    if (!has_acc) {
        acc = current;
        has_acc = 1;
        return;
    }
    switch (pending) {
        case OP_ADD: acc = acc + current; break;
        case OP_SUB: acc = acc - current; break;
        case OP_MUL: acc = acc * current; break;
        case OP_DIV:
            if (current == 0) { error = 1; return; }
            acc = acc / current;
            break;
        case OP_NONE: acc = current; break;
    }
}

static void press_digit(int d) {
    if (error) return;
    if (!entering) {
        current = 0;
        entering = 1;
    }
    if (current > 99999999) return;  /* clamp to keep display short */
    current = current * 10 + d;
}

static void press_op(op_t op) {
    if (error) return;
    if (entering) {
        apply_pending();
        entering = 0;
    } else if (!has_acc) {
        acc = current;
        has_acc = 1;
    }
    pending = op;
    current = 0;
}

static void press_equals(void) {
    if (error) return;
    if (pending != OP_NONE) {
        apply_pending();
    } else if (entering) {
        acc = current;
        has_acc = 1;
    }
    pending = OP_NONE;
    entering = 0;
    current = acc;
}

static void press_clear(void) {
    acc = 0;
    current = 0;
    pending = OP_NONE;
    entering = 0;
    has_acc = 0;
    error = 0;
}

static void press_negate(void) {
    if (error) return;
    if (entering) current = -current;
    else { acc = -acc; current = acc; }
}

static void press_percent(void) {
    if (error) return;
    long val = entering ? current : acc;
    val = val / 100;
    if (entering) current = val;
    else { acc = val; current = acc; }
}

static void press_dot(void) {
    /* Integer-only calculator — '.' is a no-op for now. */
}

/* ----- Drawing ----- */

static void compute_btn_rect(int row, int col, int *bx, int *by, int *bw, int *bh) {
    int inner_x = win_x + 16;
    int inner_y = win_y + 24 + DISPLAY_INSET + DISPLAY_H + 16;
    int inner_w = WIN_W - 32;
    int inner_h = WIN_H - (inner_y - win_y) - 16;

    int cell_w = (inner_w - (BUTTON_COLS - 1) * BUTTON_GAP) / BUTTON_COLS;
    int cell_h = (inner_h - (BUTTON_ROWS - 1) * BUTTON_GAP) / BUTTON_ROWS;

    *bx = inner_x + col * (cell_w + BUTTON_GAP);
    *by = inner_y + row * (cell_h + BUTTON_GAP);
    *bw = cell_w;
    *bh = cell_h;

    /* '0' on the bottom row spans two cells. */
    if (row == 4 && col == 0) {
        *bw = cell_w * 2 + BUTTON_GAP;
    }
}

static int hit_button(int mx, int my, int *out_row, int *out_col) {
    for (int row = 0; row < BUTTON_ROWS; row++) {
        for (int col = 0; col < BUTTON_COLS; col++) {
            const char *label = btn_grid[row][col];
            if (!label[0]) continue;
            /* Skip cell occupied by the wide '0'. */
            if (row == 4 && col == 1) continue;
            int bx, by, bw, bh;
            compute_btn_rect(row, col, &bx, &by, &bw, &bh);
            if (gui_hit(bx, by, bw, bh, mx, my)) {
                *out_row = row;
                *out_col = col;
                return 1;
            }
        }
    }
    return 0;
}

static void draw_calculator(int pressed_row, int pressed_col, int hover_row, int hover_col) {
    /* Window frame */
    gui_draw_window(win_x, win_y, WIN_W, WIN_H, "Calculator");

    /* Display panel */
    int dx = win_x + 16;
    int dy = win_y + 24 + DISPLAY_INSET;
    int dw = WIN_W - 32;
    int dh = DISPLAY_H;
    fb_fill_rect(dx, dy, dw, dh, FB_DISPLAY_BG);
    gui_draw_bevel(dx, dy, dw, dh, GUI_BEVEL_SUNKEN);
    gui_draw_bevel(dx + 1, dy + 1, dw - 2, dh - 2, GUI_BEVEL_SUNKEN);

    /* Render display value right-aligned */
    int scale = 4;
    int tw = fb_text_width(display, scale);
    int th = 8 * scale;
    int tx = dx + dw - tw - 12;
    int ty = dy + (dh - th) / 2;
    if (tx < dx + 8) tx = dx + 8;
    fb_draw_string(tx, ty, display, FB_DISPLAY_FG, scale);

    /* Button grid */
    for (int row = 0; row < BUTTON_ROWS; row++) {
        for (int col = 0; col < BUTTON_COLS; col++) {
            const char *label = btn_grid[row][col];
            if (!label[0]) continue;
            if (row == 4 && col == 1) continue;
            int bx, by, bw, bh;
            compute_btn_rect(row, col, &bx, &by, &bw, &bh);
            int pressed = (row == pressed_row && col == pressed_col);
            int hovered = (row == hover_row && col == hover_col);
            gui_draw_button(bx, by, bw, bh, label, pressed, hovered);
        }
    }
}

/* Map a (row,col) press to a calculator action. */
static void dispatch(int row, int col) {
    const char *label = btn_grid[row][col];
    if (!label[0]) return;

    if (label[0] >= '0' && label[0] <= '9' && label[1] == '\0') {
        press_digit(label[0] - '0');
    } else if (label[0] == '+' && label[1] == '\0') {
        press_op(OP_ADD);
    } else if (label[0] == '-' && label[1] == '\0') {
        press_op(OP_SUB);
    } else if (label[0] == '*') {
        press_op(OP_MUL);
    } else if (label[0] == '/') {
        press_op(OP_DIV);
    } else if (label[0] == '=') {
        press_equals();
    } else if (label[0] == 'C') {
        press_clear();
    } else if (label[0] == '.') {
        press_dot();
    } else if (label[0] == '%') {
        press_percent();
    } else if (label[0] == '+' && label[1] == '/') {
        press_negate();
    }
    update_display();
}

/* ----- Main loop ----- */

void calc_run(void) {
    win_x = (fb_width()  - WIN_W) / 2;
    win_y = (fb_height() - WIN_H) / 2;
    if (win_x < 0) win_x = 0;
    if (win_y < 24) win_y = 24;

    press_clear();
    update_display();

    int prev_row = -1, prev_col = -1;
    int pressed_row = -1, pressed_col = -1;
    int hover_row = -1, hover_col = -1;
    uint8_t prev_btns = 0;

    gui_draw_desktop();
    draw_calculator(-1, -1, -1, -1);
    gui_cursor_show(mouse_x(), mouse_y());

    while (1) {
        int mx = mouse_x();
        int my = mouse_y();
        uint8_t btns = mouse_buttons();

        int hit_row = -1, hit_col = -1;
        int over = hit_button(mx, my, &hit_row, &hit_col);

        int new_hover_row = over ? hit_row : -1;
        int new_hover_col = over ? hit_col : -1;
        int new_pressed_row = -1, new_pressed_col = -1;
        if ((btns & MOUSE_BTN_LEFT) && over) {
            new_pressed_row = hit_row;
            new_pressed_col = hit_col;
        }

        /* On left-button release while over a button, fire the action. */
        if ((prev_btns & MOUSE_BTN_LEFT) && !(btns & MOUSE_BTN_LEFT)) {
            if (pressed_row != -1 && over &&
                pressed_row == hit_row && pressed_col == hit_col) {
                dispatch(hit_row, hit_col);
            }
            new_pressed_row = -1;
            new_pressed_col = -1;
        }

        int need_redraw =
            (mx != prev_col || my != prev_row) ||
            (new_hover_row != hover_row || new_hover_col != hover_col) ||
            (new_pressed_row != pressed_row || new_pressed_col != pressed_col);

        if (need_redraw) {
            gui_cursor_hide();
            hover_row = new_hover_row;
            hover_col = new_hover_col;
            pressed_row = new_pressed_row;
            pressed_col = new_pressed_col;
            draw_calculator(pressed_row, pressed_col, hover_row, hover_col);
            gui_cursor_show(mx, my);
            prev_col = mx;
            prev_row = my;
        }

        prev_btns = btns;

        /* Be polite — sleep until the next interrupt (mouse/timer). */
        __asm__ volatile("hlt");
    }
}
