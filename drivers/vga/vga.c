/*
 * AcharyaOS - vga.c
 * ------------------
 * See vga.h for the API contract and the architectural reasoning for why
 * this lives in drivers/ rather than kernel/. This file is the ONLY place
 * in AcharyaOS allowed to know that VGA text mode exists, lives at
 * physical address 0xB8000, or uses I/O ports 0x3D4/0x3D5 for the hardware
 * cursor. Everything else goes through vga.h's functions.
 */

#include "vga.h"
#include "port_io.h"

/*
 * VGA text mode memory layout: 80x25 character cells, memory-mapped at
 * 0xB8000. Each cell is 2 bytes: [ASCII byte][attribute byte].
 * Attribute byte layout: bits 7-4 = background color, bits 3-0 = foreground
 * color (this is a hardware-fixed format, not something we chose).
 */
#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

/*
 * Hardware cursor control ports. The VGA controller has TWO registers
 * accessed through one pair of I/O ports (0x3D4 = "which register do you
 * want", 0x3D5 = "the value for that register") - this index/data port
 * pattern is common across many old PC hardware devices, not VGA-specific.
 * Register 0x0E/0x0F hold the high/low byte of the cursor's linear
 * position (row * 80 + col).
 */
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5
#define VGA_CURSOR_HIGH_REG 0x0E
#define VGA_CURSOR_LOW_REG  0x0F

static volatile uint16_t *const vga_buffer = (volatile uint16_t *) VGA_ADDRESS;

static int vga_cursor_row = 0;
static int vga_cursor_col = 0;
static uint8_t vga_current_color = (uint8_t)(VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREY;

/* Build one VGA memory cell: low byte = character, high byte = color attribute. */
static inline uint16_t vga_make_cell(char c, uint8_t color) {
    return (uint16_t)((unsigned char) c) | (uint16_t)(color << 8);
}

/*
 * vga_hw_set_cursor: push the software cursor position out to the actual
 * VGA hardware cursor, so the user sees a real blinking cursor in the
 * right place - not just characters appearing with no visible insertion
 * point. This matters starting with this feature because the upcoming
 * Command Shell needs the person typing to SEE where their next
 * keystroke will land.
 */
static void vga_hw_set_cursor(int row, int col) {
    uint16_t position = (uint16_t)(row * VGA_COLS + col);

    outb(VGA_CTRL_PORT, VGA_CURSOR_HIGH_REG);
    outb(VGA_DATA_PORT, (uint8_t)((position >> 8) & 0xFF));
    outb(VGA_CTRL_PORT, VGA_CURSOR_LOW_REG);
    outb(VGA_DATA_PORT, (uint8_t)(position & 0xFF));
}

static void vga_scroll(void) {
    for (int row = 1; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            vga_buffer[(row - 1) * VGA_COLS + col] = vga_buffer[row * VGA_COLS + col];
        }
    }
    for (int col = 0; col < VGA_COLS; col++) {
        vga_buffer[(VGA_ROWS - 1) * VGA_COLS + col] = vga_make_cell(' ', vga_current_color);
    }
    vga_cursor_row = VGA_ROWS - 1;
}

void vga_init(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_set_color(vga_color_t foreground, vga_color_t background) {
    vga_current_color = (uint8_t)((background << 4) | (foreground & 0x0F));
}

void vga_clear(void) {
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
        vga_buffer[i] = vga_make_cell(' ', vga_current_color);
    }
    vga_cursor_row = 0;
    vga_cursor_col = 0;
    vga_hw_set_cursor(vga_cursor_row, vga_cursor_col);
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_cursor_col = 0;
        vga_cursor_row++;
    } else if (c == '\r') {
        vga_cursor_col = 0;
    } else if (c == '\b') {
        /* Backspace: move back one cell and erase it. We do NOT wrap to
           the previous line on backspace at column 0 - that requires
           remembering where each line actually started (a future Command
           Shell / line-editing concern), and guessing wrong here would
           silently eat the wrong character. Out of scope for this driver;
           the shell will own correct line-editing semantics later. */
        if (vga_cursor_col > 0) {
            vga_cursor_col--;
            vga_buffer[vga_cursor_row * VGA_COLS + vga_cursor_col] =
                vga_make_cell(' ', vga_current_color);
        }
    } else {
        vga_buffer[vga_cursor_row * VGA_COLS + vga_cursor_col] = vga_make_cell(c, vga_current_color);
        vga_cursor_col++;
        if (vga_cursor_col >= VGA_COLS) {
            vga_cursor_col = 0;
            vga_cursor_row++;
        }
    }

    if (vga_cursor_row >= VGA_ROWS) {
        vga_scroll();
    }

    vga_hw_set_cursor(vga_cursor_row, vga_cursor_col);
}

void vga_set_cursor(int row, int col) {
    /* Clamp rather than trust the caller - a driver should never let bad
       input from above corrupt its own internal state or write outside
       the VGA buffer. */
    if (row < 0) row = 0;
    if (row >= VGA_ROWS) row = VGA_ROWS - 1;
    if (col < 0) col = 0;
    if (col >= VGA_COLS) col = VGA_COLS - 1;

    vga_cursor_row = row;
    vga_cursor_col = col;
    vga_hw_set_cursor(vga_cursor_row, vga_cursor_col);
}
