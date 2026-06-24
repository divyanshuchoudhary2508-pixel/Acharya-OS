/*
 * AcharyaOS - kstring.c
 * ----------------------
 * Deliberately simple, byte-at-a-time implementations. Real libc
 * implementations are heavily optimized (word-at-a-time copies, SIMD, etc)
 * but optimization is the wrong priority for a learning-focused kernel's
 * FIRST versions of these functions - clarity wins per the project's stated
 * development philosophy. We can revisit performance later once correctness
 * is proven and if profiling ever shows it matters.
 */

#include "kstring.h"

void *memset(void *dest, int value, size_t count) {
    unsigned char *d = (unsigned char *) dest;
    unsigned char v = (unsigned char) value;
    for (size_t i = 0; i < count; i++) {
        d[i] = v;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    unsigned char *d = (unsigned char *) dest;
    const unsigned char *s = (const unsigned char *) src;
    /* NOTE: like the real memcpy, this does NOT handle overlapping
       src/dest regions safely - that's what memmove is for. We don't
       have memmove yet because nothing needs it yet; adding it speculatively
       would violate "don't write code nothing calls." */
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

int memcmp(const void *a, const void *b, size_t count) {
    const unsigned char *pa = (const unsigned char *) a;
    const unsigned char *pb = (const unsigned char *) b;
    for (size_t i = 0; i < count; i++) {
        if (pa[i] != pb[i]) {
            /* Match the standard contract: sign of (a[i] - b[i]), not just -1/0/1 */
            return (int) pa[i] - (int) pb[i];
        }
    }
    return 0;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char) *a - (unsigned char) *b;
}
