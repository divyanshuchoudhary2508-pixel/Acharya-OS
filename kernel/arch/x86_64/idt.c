/*
 * AcharyaOS - idt.c
 * -----------------
 * Builds and loads the kernel IDT. For this milestone we install CPU
 * exception vectors 0-31 and legacy PIC IRQ vectors 32-47, which is enough
 * for the keyboard and timer-era infrastructure without generating unused
 * boilerplate for every possible vector.
 */

#include "idt.h"
#include "kernel.h"
#include "kio.h"

#define IDT_ENTRY_COUNT 256
#define IDT_PRESENT     0x80
#define IDT_GATE_INT64  0x0E
#define IDT_RING3_GATE  0xEE
#define KERNEL_CS       0x08

typedef struct PACKED {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} idt_entry_t;

typedef struct PACKED {
    uint16_t limit;
    uint64_t base;
} idt_pointer_t;

static idt_entry_t idt[IDT_ENTRY_COUNT];
static idt_pointer_t idt_pointer;
static interrupt_handler_t handlers[IDT_ENTRY_COUNT];

extern void idt_load(uint64_t idt_pointer_address);

extern void isr_stub_0(void);  extern void isr_stub_1(void);
extern void isr_stub_2(void);  extern void isr_stub_3(void);
extern void isr_stub_4(void);  extern void isr_stub_5(void);
extern void isr_stub_6(void);  extern void isr_stub_7(void);
extern void isr_stub_8(void);  extern void isr_stub_9(void);
extern void isr_stub_10(void); extern void isr_stub_11(void);
extern void isr_stub_12(void); extern void isr_stub_13(void);
extern void isr_stub_14(void); extern void isr_stub_15(void);
extern void isr_stub_16(void); extern void isr_stub_17(void);
extern void isr_stub_18(void); extern void isr_stub_19(void);
extern void isr_stub_20(void); extern void isr_stub_21(void);
extern void isr_stub_22(void); extern void isr_stub_23(void);
extern void isr_stub_24(void); extern void isr_stub_25(void);
extern void isr_stub_26(void); extern void isr_stub_27(void);
extern void isr_stub_28(void); extern void isr_stub_29(void);
extern void isr_stub_30(void); extern void isr_stub_31(void);
extern void isr_stub_32(void); extern void isr_stub_33(void);
extern void isr_stub_34(void); extern void isr_stub_35(void);
extern void isr_stub_36(void); extern void isr_stub_37(void);
extern void isr_stub_38(void); extern void isr_stub_39(void);
extern void isr_stub_40(void); extern void isr_stub_41(void);
extern void isr_stub_42(void); extern void isr_stub_43(void);
extern void isr_stub_44(void); extern void isr_stub_45(void);
extern void isr_stub_46(void); extern void isr_stub_47(void);
extern void isr_stub_128(void);

void idt_install_gate(uint8_t vector, uint64_t handler_address, uint8_t type_attr) {
    idt[vector].offset_low  = (uint16_t)(handler_address & 0xFFFF);
    idt[vector].selector    = KERNEL_CS;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (uint16_t)((handler_address >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((handler_address >> 32) & 0xFFFFFFFF);
    idt[vector].zero        = 0;
}

static void idt_install_default_gates(void) {
    void (*stubs[48])(void) = {
        isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3,
        isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7,
        isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11,
        isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15,
        isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19,
        isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23,
        isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27,
        isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31,
        isr_stub_32, isr_stub_33, isr_stub_34, isr_stub_35,
        isr_stub_36, isr_stub_37, isr_stub_38, isr_stub_39,
        isr_stub_40, isr_stub_41, isr_stub_42, isr_stub_43,
        isr_stub_44, isr_stub_45, isr_stub_46, isr_stub_47
    };

    for (uint8_t vector = 0; vector < 48; vector++) {
        idt_install_gate(vector, (uint64_t) stubs[vector], IDT_PRESENT | IDT_GATE_INT64);
    }

    idt_install_gate(128, (uint64_t) isr_stub_128, IDT_PRESENT | IDT_GATE_INT64);
}

void idt_register_handler(uint8_t vector, interrupt_handler_t handler) {
    handlers[vector] = handler;
}

void idt_dispatch(interrupt_frame_t *frame) {
    interrupt_handler_t handler = handlers[frame->vector];
    if (handler) {
        handler(frame);
        return;
    }

    if (frame->vector < IDT_IRQ_BASE) {
        kprintf("\n[PANIC] Unhandled CPU exception vector %d, error %x\n",
                (int32_t) frame->vector, (uint32_t) frame->error_code);
        kernel_panic("Unhandled CPU exception");
    }

    kprintf("[warn] Unhandled IRQ vector %d\n", (int32_t) frame->vector);
}

void idt_init(void) {
    for (int i = 0; i < IDT_ENTRY_COUNT; i++) {
        idt[i] = (idt_entry_t) {0};
        handlers[i] = 0;
    }

    idt_install_default_gates();

    idt_pointer.limit = (uint16_t)(sizeof(idt) - 1);
    idt_pointer.base = (uint64_t) &idt;
    idt_load((uint64_t) &idt_pointer);
}
