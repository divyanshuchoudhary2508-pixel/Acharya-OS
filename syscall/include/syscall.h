/*
 * AcharyaOS - syscall.h
 * ---------------------
 * Phase 1, Feature 11: system call interface. AcharyaOS uses an `int 0x80`
 * software interrupt gate for now because it is simple, explicit, and easy to
 * debug in a teaching kernel.
 */

#ifndef ACHARYAOS_SYSCALL_H
#define ACHARYAOS_SYSCALL_H

#include <stdint.h>

typedef enum {
    SYSCALL_PUTS = 0,
    SYSCALL_GET_TICKS = 1,
    SYSCALL_GET_PID = 2,
    SYSCALL_GET_PROCESS_COUNT = 3,
    SYSCALL_YIELD = 4,
} syscall_number_t;

void syscall_init(void);
void syscall_write(const char *str);
uint64_t syscall_get_ticks(void);
uint32_t syscall_get_pid(void);
uint32_t syscall_get_process_count(void);
uint64_t syscall_yield(void);

#endif /* ACHARYAOS_SYSCALL_H */
