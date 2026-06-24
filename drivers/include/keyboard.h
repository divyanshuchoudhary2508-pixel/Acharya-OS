/*
 * AcharyaOS - keyboard.h
 * ----------------------
 * PS/2 keyboard driver API. The interrupt handler translates scancodes into
 * ASCII and queues them; callers consume characters with keyboard_getchar().
 */

#ifndef ACHARYAOS_KEYBOARD_H
#define ACHARYAOS_KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
int keyboard_getchar(void);

#endif /* ACHARYAOS_KEYBOARD_H */
