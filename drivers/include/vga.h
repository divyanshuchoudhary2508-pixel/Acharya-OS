/*
 * AcharyaOS - vga.h
 * ------------------
 * Phase 1, Feature 3: Text Output.
 *
 * This is the REAL driver, living in drivers/ as required by the project
 * structure - not kernel/lib/kio.c anymore. kio.c (Feature 2) was always
 * documented as a temporary seed; this header is where that debt gets
 * paid off.
 *
 * WHY THIS SEPARATION MATTERS (not just structure for its own sake):
 * Every piece of VGA-specific hardware knowledge - the 0xB8000 memory
 * layout, the color attribute byte format, the hardware cursor I/O ports -
 * lives ONLY in vga.c. Nothing outside drivers/vga/ should know any of
 * that. When Phase 3 (GUI) eventually wants a framebuffer-based display
 * instead of VGA text mode, the goal is that ONLY this driver needs
 * replacing - kio.c and everything built on top of kprintf/kputs should
 * not need to change at all, because they only ever call this header's
 * functions, never touch 0xB8000 directly.
 */

#ifndef ACHARYAOS_VGA_H
#define ACHARYAOS_VGA_H

#include <stdint.h>

/*
 * VGA text-mode color palette. These are the 16 standard colors the
 * hardware supports natively - this is a hardware fact, not a design
 * choice we made, so the enum values must match the VGA attribute byte
 * encoding exactly (0x0 - 0xF).
 */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15
} vga_color_t;

/* Initialize the driver: clear the screen, reset cursor to (0,0), set the
   default color, enable and position the hardware cursor. Must be called
   once before any other vga_* function. */
void vga_init(void);

/* Write a single character at the current cursor position, advancing the
   cursor (handling line wrap and scrolling), using the CURRENT foreground/
   background color set by vga_set_color(). */
void vga_putchar(char c);

/* Change the foreground/background color used by subsequent vga_putchar
   calls. Does not affect characters already on screen - matches how real
   terminals behave (changing color mid-stream affects only new output). */
void vga_set_color(vga_color_t foreground, vga_color_t background);

/* Clear the entire screen using the CURRENT color, reset cursor to (0,0). */
void vga_clear(void);

/* Explicitly move the cursor to a specific row/column. Used by the future
   Command Shell (e.g. for line-editing) and Task Manager, not needed by
   plain sequential output. Out-of-range values are clamped, not undefined
   behavior - a driver should never let a caller crash the display. */
void vga_set_cursor(int row, int col);

#endif /* ACHARYAOS_VGA_H */
