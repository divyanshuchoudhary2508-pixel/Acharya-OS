/*
 * AcharyaOS - kstring.h
 * ----------------------
 * A freestanding kernel has NO libc - no memset, no memcpy, no strlen.
 * These aren't optional conveniences; the C compiler itself silently
 * generates CALLS to memset/memcpy for certain patterns (e.g. zeroing a
 * large local struct, struct assignment) even if you never call them
 * yourself. Without our own implementations, the linker would fail with
 * "undefined reference to memset" on code that looks like it shouldn't
 * need it at all.
 *
 * We implement only what we need, when we need it - this is the bare
 * minimum set required by the Kernel subsystem. Expect this file to grow
 * (strcmp, strcpy, etc) as later subsystems need them, rather than
 * front-loading functions nothing calls yet.
 */

#ifndef ACHARYAOS_KSTRING_H
#define ACHARYAOS_KSTRING_H

#include <stddef.h>

void *memset(void *dest, int value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
int memcmp(const void *a, const void *b, size_t count);
size_t strlen(const char *str);
int strcmp(const char *a, const char *b);

#endif /* ACHARYAOS_KSTRING_H */
