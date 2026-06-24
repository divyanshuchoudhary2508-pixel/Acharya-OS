/*
 * AcharyaOS - pic.h
 * -----------------
 * Legacy 8259 PIC support. The PIC sits between hardware IRQ lines and the
 * CPU's IDT vectors. We remap IRQ0-15 to vectors 32-47 so they do not collide
 * with CPU exceptions 0-31.
 */

#ifndef ACHARYAOS_PIC_H
#define ACHARYAOS_PIC_H

#include <stdint.h>

#define PIC_IRQ_BASE 32

void pic_init(void);
void pic_unmask_irq(uint8_t irq);
void pic_send_eoi(uint8_t irq);

#endif /* ACHARYAOS_PIC_H */
