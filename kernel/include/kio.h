/*
 * AcharyaOS - kio.h
 * ------------------
 * "kio" = kernel I/O. This is NOT the Text Output subsystem (Phase 1,
 * Feature 3) - that future driver will own cursor tracking, scrolling,
 * colors, and a clean abstraction other subsystems call into. THIS module
 * exists only so the kernel's OWN boot-time diagnostics have one shared,
 * non-duplicated place to live, instead of being copy-pasted raw VGA/serial
 * pokes scattered across every .c file (which is what the bootloader's
 * stub kmain.c did, and which we're now cleaning up).
 *
 * Design choice: kprintf supports a deliberately TINY format spec right
 * now (%s, %d, %x, %c, %%) - just enough for boot diagnostics. This is not
 * a libc printf clone. Expanding it is explicitly in-scope for the future
 * Text Output / Logging subsystems, not this one.
 */

#ifndef ACHARYAOS_KIO_H
#define ACHARYAOS_KIO_H

#include "vga.h"

/* Must be called once, before any other kio function, to initialize the
   serial port and clear the VGA screen. */
void kio_init(void);

/*
 * kio_set_color: thin pass-through to the VGA driver's color control.
 * Exposed here (rather than making every caller #include vga.h directly)
 * because most callers think in terms of "kernel output," not "the VGA
 * driver specifically" - kio.h is the kernel's intended single entry
 * point for all of its own diagnostic output, color included.
 */
void kio_set_color(vga_color_t foreground, vga_color_t background);

/* Print a raw string with no formatting, to both VGA and serial. */
void kputs(const char *str);

/*
 * kprintf: minimal formatted output. Supported specifiers:
 *   %s  - null-terminated string
 *   %d  - signed decimal integer (32-bit)
 *   %x  - unsigned hexadecimal integer (32-bit), lowercase, no "0x" prefix
 *   %c  - single character
 *   %%  - literal '%'
 * Anything else is printed as-is (specifier letter included) rather than
 * silently eaten - a wrong format string should be visible, not hidden.
 */
void kprintf(const char *fmt, ...);

#endif /* ACHARYAOS_KIO_H */
