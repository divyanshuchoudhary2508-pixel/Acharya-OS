/*
 * AcharyaOS - gdt_flush.S
 * ------------------------
 * gdt_flush: load a new GDT pointer and reload the segment registers to
 * point at the new table's entries.
 *
 * WHY this can't be written in C: reloading CS (the code segment register)
 * requires a far call/jump or a similar control-transfer instruction -
 * there's no plain "mov" form for CS. C has no syntax for that, so this
 * one small piece must be hand-written assembly, called from gdt_init().
 *
 * Calling convention: gdt_flush(uint64_t gdt_pointer_address) - per x86_64
 * System V ABI, the first argument arrives in RDI.
 */
.section .text
.code64
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    lgdt (%rdi)              /* load the GDT pointer struct at address RDI */

    /* Reload data-segment-family registers directly - these accept a
       normal mov into the register. Selector 0x10 = index 2 (kernel data)
       in our 5-entry table; index * 8 because each GDT entry is 8 bytes. */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    /*
     * Reload CS (selector 0x08 = index 1, kernel code) via a far return.
     * This is the standard trick for reloading CS in 64-bit mode without
     * actually transferring control elsewhere: push a target selector and
     * return address, then `lretq` (far return) "returns" to the very
     * next instruction, but in doing so it loads CS from the value we
     * pushed. It looks unusual but is the conventional, documented
     * technique used by essentially every x86_64 kernel for this exact
     * purpose.
     */
    push $0x08
    push $.gdt_flush_reload_cs
    lretq

.gdt_flush_reload_cs:
    ret

.global gdt_load_tss
.type gdt_load_tss, @function
gdt_load_tss:
    mov $0x28, %ax
    ltr %ax
    ret
