/*
 * AcharyaOS - kernel.h
 * ---------------------
 * Top-level kernel header. Anything genuinely shared across MULTIPLE
 * subsystems (not just one .c/.h pair) belongs here. Keep this file small -
 * if it grows into a dumping ground for every typedef in the OS, it becomes
 * a hidden coupling point between unrelated subsystems, which fights the
 * "loosely coupled modules" principle directly.
 *
 * WHY freestanding kernels need their own typedefs:
 * In a normal hosted C program, <stdint.h>/<stddef.h> come from libc and
 * just work. We ARE allowed to use <stdint.h> and <stddef.h> specifically
 * because those are pure compiler-provided headers (no runtime functions,
 * just type definitions) - GCC ships these itself, independent of libc.
 * That's why you'll see us include them directly rather than hand-rolling
 * uint8_t/uint32_t/etc ourselves.
 */

#ifndef ACHARYAOS_KERNEL_H
#define ACHARYAOS_KERNEL_H

#include <stdint.h>
#include <stddef.h>

/*
 * NORETURN: tells GCC a function never returns (e.g. kmain, panic handlers).
 * This lets the compiler warn us if we accidentally fall off the end of a
 * function that should have halted the CPU forever - a real bug class in
 * freestanding code where there's no "return to the OS" safety net.
 */
#define NORETURN __attribute__((noreturn))

/*
 * PACKED: tells GCC not to insert padding bytes between struct members.
 * Critical for hardware-defined structures (GDT entries, IDT entries,
 * Multiboot2 info tables) where the EXACT byte layout is dictated by a
 * specification, not by what's convenient for the compiler.
 */
#define PACKED __attribute__((packed))

/*
 * kernel_panic: the freestanding-kernel equivalent of "abort()". Used when
 * we detect a condition we cannot safely continue from (this is intentionally
 * just a declaration here - the implementation lives in kmain.c since it
 * needs access to kprintf, and we want to avoid a circular include).
 */
void kernel_panic(const char *message) NORETURN;

#endif /* ACHARYAOS_KERNEL_H */
