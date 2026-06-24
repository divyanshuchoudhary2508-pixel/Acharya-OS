/*
 * AcharyaOS - syscall.c
 * ---------------------
 * `int 0x80` syscall entry. The kernel can call these wrappers too, which
 * makes Feature 11 testable before userspace exists.
 */

#include "syscall.h"
#include "idt.h"
#include "kio.h"
#include "process.h"
#include "timer.h"

#define SYSCALL_VECTOR 128
#define SYSCALL_GATE_ATTR 0xEE

extern void isr_stub_128(void);

static void syscall_dispatch(interrupt_frame_t *frame) {
    switch (frame->rax) {
        case SYSCALL_PUTS:
            kputs((const char *) frame->rdi);
            frame->rax = 0;
            break;
        case SYSCALL_GET_TICKS:
            frame->rax = timer_get_ticks();
            break;
        case SYSCALL_GET_PID:
            frame->rax = process_current_pid();
            break;
        case SYSCALL_GET_PROCESS_COUNT:
            frame->rax = process_get_count();
            break;
        case SYSCALL_YIELD:
            frame->rax = 0;
            break;
        default:
            frame->rax = (uint64_t) -1;
            break;
    }
}

void syscall_init(void) {
    idt_install_gate(SYSCALL_VECTOR, (uint64_t) &isr_stub_128, SYSCALL_GATE_ATTR);
    idt_register_handler(SYSCALL_VECTOR, syscall_dispatch);
}

void syscall_write(const char *str) {
    __asm__ volatile (
        "movq %[num], %%rax\n\t"
        "movq %[arg], %%rdi\n\t"
        "int $0x80\n\t"
        :
        : [num] "r" ((uint64_t) SYSCALL_PUTS), [arg] "r" (str)
        : "rax", "rdi", "memory"
    );
}

uint64_t syscall_get_ticks(void) {
    uint64_t result;
    __asm__ volatile (
        "movq %[num], %%rax\n\t"
        "int $0x80\n\t"
        : "=a" (result)
        : [num] "r" ((uint64_t) SYSCALL_GET_TICKS)
        : "memory"
    );
    return result;
}

uint32_t syscall_get_pid(void) {
    uint64_t result;
    __asm__ volatile (
        "movq %[num], %%rax\n\t"
        "int $0x80\n\t"
        : "=a" (result)
        : [num] "r" ((uint64_t) SYSCALL_GET_PID)
        : "memory"
    );
    return (uint32_t) result;
}

uint32_t syscall_get_process_count(void) {
    uint64_t result;
    __asm__ volatile (
        "movq %[num], %%rax\n\t"
        "int $0x80\n\t"
        : "=a" (result)
        : [num] "r" ((uint64_t) SYSCALL_GET_PROCESS_COUNT)
        : "memory"
    );
    return (uint32_t) result;
}

uint64_t syscall_yield(void) {
    uint64_t result;
    __asm__ volatile (
        "movq %[num], %%rax\n\t"
        "int $0x80\n\t"
        : "=a" (result)
        : [num] "r" ((uint64_t) SYSCALL_YIELD)
        : "memory"
    );
    return result;
}
