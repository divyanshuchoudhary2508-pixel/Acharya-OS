/*
 * AcharyaOS - mouse.c
 * -------------------
 * Phase 3, Feature 28: mouse driver.
 *
 * The PS/2 mouse uses the auxiliary device channel on the same controller as
 * the keyboard. The controller is programmed to enable the auxiliary device,
 * unmask IRQ12, and then collect 3-byte packets in an interrupt handler.
 */

#include "mouse.h"
#include "idt.h"
#include "pic.h"
#include "port_io.h"

#define PS2_STATUS_PORT      0x64
#define PS2_DATA_PORT        0x60
#define PS2_CMD_WRITE        0xD4
#define PS2_CMD_ENABLE_AUX   0xA8
#define PS2_CMD_SET_DEFAULTS 0xF6
#define PS2_CMD_ENABLE_DATA  0xF4

#define MOUSE_IRQ 12

static mouse_state_t mouse_state;
static uint8_t mouse_packet[3];
static uint8_t mouse_index = 0;
static int mouse_has_packet = 0;

static void mouse_wait_input(void) {
    while ((inb(PS2_STATUS_PORT) & 0x02) != 0) {
    }
}

static void mouse_wait_output(void) {
    while ((inb(PS2_STATUS_PORT) & 0x01) == 0) {
    }
}

static void mouse_write(uint8_t value) {
    mouse_wait_input();
    outb(PS2_STATUS_PORT, PS2_CMD_WRITE);
    mouse_wait_input();
    outb(PS2_DATA_PORT, value);
}

static void mouse_interrupt_handler(interrupt_frame_t *frame) {
    (void) frame;

    uint8_t data = inb(PS2_DATA_PORT);

    if (mouse_index == 0 && (data & 0x08) == 0) {
        pic_send_eoi(MOUSE_IRQ);
        return;
    }

    mouse_packet[mouse_index++] = data;
    if (mouse_index >= 3) {
        mouse_index = 0;
        mouse_state.buttons = mouse_packet[0] & 0x07;
        mouse_state.dx = (int8_t) mouse_packet[1];
        mouse_state.dy = (int8_t) mouse_packet[2];
        mouse_state.x += mouse_state.dx;
        mouse_state.y -= mouse_state.dy;
        mouse_state.packet_count++;
        mouse_state.packet_ready = 1;
        mouse_has_packet = 1;
    }

    pic_send_eoi(MOUSE_IRQ);
}

void mouse_init(void) {
    mouse_state.ready = 0;
    mouse_state.packet_ready = 0;
    mouse_state.x = 0;
    mouse_state.y = 0;
    mouse_state.dx = 0;
    mouse_state.dy = 0;
    mouse_state.buttons = 0;
    mouse_state.packet_count = 0;
    mouse_index = 0;
    mouse_has_packet = 0;

    mouse_wait_input();
    outb(PS2_STATUS_PORT, PS2_CMD_ENABLE_AUX);
    mouse_wait_input();
    outb(PS2_STATUS_PORT, 0x20);
    mouse_wait_output();
    (void) inb(PS2_DATA_PORT);
    mouse_wait_input();
    outb(PS2_STATUS_PORT, 0x60);
    mouse_wait_input();
    outb(PS2_DATA_PORT, 0x47);
    mouse_write(PS2_CMD_SET_DEFAULTS);
    mouse_write(PS2_CMD_ENABLE_DATA);

    idt_register_handler(IDT_IRQ_BASE + MOUSE_IRQ, mouse_interrupt_handler);
    pic_unmask_irq(MOUSE_IRQ);
    mouse_state.ready = 1;
}

void mouse_get_state(mouse_state_t *state) {
    if (!state) {
        return;
    }
    *state = mouse_state;
}

int mouse_ready(void) {
    return mouse_state.ready;
}

int mouse_poll_packet(uint8_t packet[3]) {
    if (!packet || !mouse_has_packet) {
        return 0;
    }
    packet[0] = mouse_packet[0];
    packet[1] = mouse_packet[1];
    packet[2] = mouse_packet[2];
    mouse_has_packet = 0;
    mouse_state.packet_ready = 0;
    return 1;
}
