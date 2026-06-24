/*
 * AcharyaOS - port_io.h
 * ----------------------
 * x86 "port I/O" is a separate address space from memory addresses - certain
 * hardware (serial controllers, the PIC, PS/2 keyboard controller, etc) is
 * accessed via special `in`/`out` CPU instructions rather than normal
 * memory reads/writes. This header declares the thin wrappers around those
 * instructions.
 *
 * This used to live inline inside kmain.c (written there only to verify the
 * bootloader). Now that we have a real driver-adjacent need (serial logging
 * inside kio.c) AND will need it again very soon for the Keyboard Driver
 * subsystem, it has earned its own shared module rather than being
 * duplicated or left buried in a file that shouldn't own it.
 */

#ifndef ACHARYAOS_PORT_IO_H
#define ACHARYAOS_PORT_IO_H

#include <stdint.h>

/* Send one byte to an I/O port. */
void outb(uint16_t port, uint8_t value);

/* Read one byte from an I/O port. (Not used yet - first consumer will be
   the Keyboard Driver subsystem, reading scancodes from port 0x60.) */
uint8_t inb(uint16_t port);

#endif /* ACHARYAOS_PORT_IO_H */
