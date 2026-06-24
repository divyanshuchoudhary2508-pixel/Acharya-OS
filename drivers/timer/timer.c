/*
 * AcharyaOS - timer.c
 * -------------------
 * Programs the legacy PIT (Programmable Interval Timer) channel 0. Like the
 * PIC, the PIT is old PC hardware, but it is perfect for Phase 1: simple,
 * well-emulated by QEMU, and enough to drive a first scheduler tick.
 */

#include "timer.h"
#include "idt.h"
#include "pic.h"
#include "port_io.h"
#include "scheduler.h"

#define PIT_INPUT_HZ       1193182u
#define PIT_COMMAND_PORT   0x43
#define PIT_CHANNEL0_PORT  0x40
#define PIT_MODE3_COMMAND  0x36
#define TIMER_IRQ          0
#define TIMER_VECTOR       (PIC_IRQ_BASE + TIMER_IRQ)

static volatile uint64_t ticks;
static uint32_t configured_frequency;

static void timer_interrupt_handler(interrupt_frame_t *frame) {
    (void) frame;
    ticks++;
    scheduler_on_timer_tick();
    pic_send_eoi(TIMER_IRQ);
}

void timer_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) {
        frequency_hz = 100;
    }

    uint32_t divisor = PIT_INPUT_HZ / frequency_hz;
    if (divisor == 0) {
        divisor = 1;
    }
    if (divisor > 0xFFFFu) {
        divisor = 0xFFFFu;
    }

    configured_frequency = PIT_INPUT_HZ / divisor;
    ticks = 0;

    idt_register_handler(TIMER_VECTOR, timer_interrupt_handler);

    outb(PIT_COMMAND_PORT, PIT_MODE3_COMMAND);
    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8) & 0xFF));

    pic_unmask_irq(TIMER_IRQ);
}

uint64_t timer_get_ticks(void) {
    return ticks;
}

uint32_t timer_get_frequency(void) {
    return configured_frequency;
}
