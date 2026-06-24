/*
 * AcharyaOS - kio.c
 * ------------------
 * See kio.h for what this module is and (importantly) what it is NOT.
 *
 * UPDATED for Phase 1, Feature 3 (Text Output): this file no longer
 * contains ANY VGA hardware knowledge. That all moved to drivers/vga/,
 * which is the real driver this module was always meant to be replaced
 * by (this was called out explicitly as a known limitation in Feature 2's
 * documentation). kio.c is now correctly just a thin layer that:
 *   (a) duplicates every character to BOTH the VGA driver and the serial
 *       port (kio's one remaining genuine job - "boot diagnostics go to
 *       two places at once" is a kernel-level policy, not a driver concern)
 *   (b) implements kprintf's minimal format-string logic, which has
 *       nothing to do with VGA OR serial specifically and shouldn't move
 *       into either.
 */

#include "kio.h"
#include "port_io.h"
#include "kstring.h"
#include "vga.h"
#include <stdarg.h>
#include <stdint.h>

/* ---------------- Serial (COM1) backend ---------------- */
/* This stays here, not in drivers/, because unlike VGA it isn't a display
   device with its own rich feature set the kernel is wrapping - it's a
   one-instruction-deep debug log sink. If a future subsystem (Networking?
   a real serial driver for external hardware?) ever needs more than raw
   byte-out, THAT would be the point to promote this into drivers/serial/.
   Until then, moving it would be structure for its own sake rather than
   solving a real coupling problem. */
#define COM1_PORT 0x3F8

static void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

static void serial_putchar(char c) {
    outb(COM1_PORT, (uint8_t) c);
}

/* ---------------- Public API ---------------- */

void kio_init(void) {
    serial_init();
    vga_init();
}

void kio_set_color(vga_color_t foreground, vga_color_t background) {
    vga_set_color(foreground, background);
}

static void kputchar(char c) {
    vga_putchar(c);
    serial_putchar(c);
}

void kputs(const char *str) {
    while (*str) {
        kputchar(*str);
        str++;
    }
}

/* Print an unsigned integer in an arbitrary base (used for %d and %x).
   Handles 0 explicitly since the digit-extraction loop below would
   otherwise print nothing for it. */
static void kprint_uint(uint32_t value, uint32_t base, int uppercase) {
    char buffer[32];     /* plenty for a 32-bit value in base 2 (worst case) */
    int pos = 0;

    if (value == 0) {
        kputchar('0');
        return;
    }

    while (value > 0) {
        uint32_t digit = value % base;
        char digit_char;
        if (digit < 10) {
            digit_char = (char)('0' + digit);
        } else {
            digit_char = (char)((uppercase ? 'A' : 'a') + (digit - 10));
        }
        buffer[pos++] = digit_char;
        value /= base;
    }

    /* Digits were extracted least-significant-first; print in reverse. */
    while (pos > 0) {
        kputchar(buffer[--pos]);
    }
}

static void kprint_int(int32_t value) {
    if (value < 0) {
        kputchar('-');
        /* Careful: -value on INT32_MIN overflows. Cast through unsigned to
           sidestep that classic bug rather than ignoring it. */
        kprint_uint((uint32_t)(-(int64_t)value), 10, 0);
    } else {
        kprint_uint((uint32_t) value, 10, 0);
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            kputchar(*fmt);
            fmt++;
            continue;
        }

        fmt++; /* consume '%' */
        switch (*fmt) {
            case 's': {
                const char *s = va_arg(args, const char *);
                kputs(s);
                break;
            }
            case 'd': {
                int32_t v = va_arg(args, int32_t);
                kprint_int(v);
                break;
            }
            case 'x': {
                uint32_t v = va_arg(args, uint32_t);
                kprint_uint(v, 16, 0);
                break;
            }
            case 'c': {
                /* va_arg with char promotes to int per C variadic rules. */
                char c = (char) va_arg(args, int);
                kputchar(c);
                break;
            }
            case '%': {
                kputchar('%');
                break;
            }
            case '\0': {
                /* Trailing '%' at end of string with nothing after it -
                   print it literally and stop, rather than reading past
                   the end of the format string. */
                kputchar('%');
                goto done;
            }
            default: {
                /* Unknown specifier: print both characters literally so
                   the mistake is visible instead of silently swallowed. */
                kputchar('%');
                kputchar(*fmt);
                break;
            }
        }
        fmt++;
    }

done:
    va_end(args);
}
