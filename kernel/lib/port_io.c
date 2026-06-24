/*
 * AcharyaOS - port_io.c
 * ----------------------
 * See port_io.h for the "why". Implementation is two inline-asm wrappers.
 */

#include "port_io.h"

void outb(uint16_t port, uint8_t value) {
    /* "a"(value) constrains value into AL (the low byte of EAX), because
       the x86 `out` instruction's 8-bit form always reads from AL.
       "Nd"(port) allows the port number to be encoded as an immediate
       (N) if it fits in 8 bits, or placed in DX (d) otherwise - this
       matches what the `out` instruction's encoding actually supports. */
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}
