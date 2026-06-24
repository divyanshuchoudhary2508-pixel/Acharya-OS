.section .text
.global idt_load
.type idt_load, @function

/* void idt_load(uint64_t idt_pointer_address)
 * RDI holds the pointer under the x86_64 System V calling convention. */
idt_load:
    lidt (%rdi)
    ret
