/*
 * AcharyaOS - mouse.h
 * -------------------
 * Phase 3, Feature 28: mouse driver.
 *
 * This driver decodes the legacy PS/2 auxiliary device packets and keeps a
 * small state record for the shell to inspect. It is intentionally compact:
 * the goal is to prove input capture and packet decoding before any GUI
 * interaction policy is built on top.
 */

#ifndef ACHARYAOS_MOUSE_H
#define ACHARYAOS_MOUSE_H

#include <stdint.h>

typedef struct {
    int ready;
    int packet_ready;
    int32_t x;
    int32_t y;
    int8_t dx;
    int8_t dy;
    uint8_t buttons;
    uint32_t packet_count;
} mouse_state_t;

void mouse_init(void);
void mouse_get_state(mouse_state_t *state);
int mouse_ready(void);
int mouse_poll_packet(uint8_t packet[3]);

#endif /* ACHARYAOS_MOUSE_H */
