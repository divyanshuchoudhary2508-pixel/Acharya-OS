/*
 * AcharyaOS - pic.c
 * -----------------
 * Programs the master/slave 8259 PIC pair. This is deliberately simple:
 * Phase 1 uses the legacy PIC because it is enough for QEMU, keyboard input,
 * and the upcoming timer interrupt before AcharyaOS needs APIC complexity.
 */

#include "pic.h"
#include "port_io.h"

#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT    0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT    0xA1

#define PIC_EOI_COMMAND   0x20

void pic_init(void) {
    (void) inb(PIC1_DATA_PORT);
    (void) inb(PIC2_DATA_PORT);

    outb(PIC1_COMMAND_PORT, 0x11);
    outb(PIC2_COMMAND_PORT, 0x11);

    outb(PIC1_DATA_PORT, PIC_IRQ_BASE);
    outb(PIC2_DATA_PORT, PIC_IRQ_BASE + 8);

    outb(PIC1_DATA_PORT, 0x04);
    outb(PIC2_DATA_PORT, 0x02);

    outb(PIC1_DATA_PORT, 0x01);
    outb(PIC2_DATA_PORT, 0x01);

    outb(PIC1_DATA_PORT, 0xFF);
    outb(PIC2_DATA_PORT, 0xFF);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;

    if (irq < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        irq -= 8;
    }

    uint8_t mask = inb(port);
    outb(port, (uint8_t)(mask & ~(1u << irq)));
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND_PORT, PIC_EOI_COMMAND);
    }
    outb(PIC1_COMMAND_PORT, PIC_EOI_COMMAND);
}
