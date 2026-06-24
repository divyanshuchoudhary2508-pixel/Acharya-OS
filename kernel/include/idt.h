/*
 * AcharyaOS - idt.h
 * -----------------
 * The Interrupt Descriptor Table (IDT) tells the CPU where to jump when an
 * exception or hardware interrupt occurs. Feature 4 uses it for the keyboard:
 * the PS/2 controller raises IRQ1, the PIC maps IRQ1 to vector 33, and the
 * CPU enters the vector-33 IDT gate.
 */

#ifndef ACHARYAOS_IDT_H
#define ACHARYAOS_IDT_H

#include <stdint.h>

#define IDT_IRQ_BASE 32
#define IDT_KEYBOARD_VECTOR (IDT_IRQ_BASE + 1)

typedef struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

typedef void (*interrupt_handler_t)(interrupt_frame_t *frame);

void idt_init(void);
void idt_register_handler(uint8_t vector, interrupt_handler_t handler);
void idt_install_gate(uint8_t vector, uint64_t handler_address, uint8_t type_attr);

#endif /* ACHARYAOS_IDT_H */
