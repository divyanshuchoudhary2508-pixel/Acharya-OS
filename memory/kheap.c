/*
 * AcharyaOS - kheap.c
 * -------------------
 * Early bump allocator.
 *
 * A bump allocator is the simplest possible heap: keep a cursor, align it,
 * hand out bytes, move the cursor forward. There is no reuse yet. kfree()
 * exists as an API placeholder so future code can compile against a normal
 * allocator shape, but it intentionally does nothing until Feature 7/Memory
 * work gives us real page ownership and lifetime rules.
 */

#include "kheap.h"
#include "kstring.h"

#define KHEAP_SIZE_BYTES (1024u * 1024u)
#define KHEAP_ALIGNMENT  16u

extern uint8_t __kernel_end;

static uintptr_t heap_start;
static uintptr_t heap_current;
static uintptr_t heap_end;
static size_t allocation_count;

static uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

void kheap_init(void) {
    heap_start = align_up((uintptr_t) &__kernel_end, 4096u);
    heap_current = heap_start;
    heap_end = heap_start + KHEAP_SIZE_BYTES;
    allocation_count = 0;
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }

    uintptr_t allocation = align_up(heap_current, KHEAP_ALIGNMENT);
    uintptr_t next = allocation + size;

    if (next < allocation || next > heap_end) {
        return 0;
    }

    heap_current = next;
    allocation_count++;
    return (void *) allocation;
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void *ptr) {
    (void) ptr;
}

void kheap_get_stats(kheap_stats_t *stats) {
    if (!stats) {
        return;
    }

    stats->heap_start = heap_start;
    stats->heap_current = heap_current;
    stats->heap_end = heap_end;
    stats->bytes_used = (size_t)(heap_current - heap_start);
    stats->allocation_count = allocation_count;
}

int kheap_self_test(void) {
    uint8_t *a = (uint8_t *) kmalloc(32);
    uint8_t *b = (uint8_t *) kzalloc(64);

    if (!a || !b) {
        return 0;
    }
    if (((uintptr_t) a % KHEAP_ALIGNMENT) != 0 ||
        ((uintptr_t) b % KHEAP_ALIGNMENT) != 0) {
        return 0;
    }
    if (b <= a) {
        return 0;
    }
    for (int i = 0; i < 64; i++) {
        if (b[i] != 0) {
            return 0;
        }
    }

    a[0] = 0xA5;
    a[31] = 0x5A;
    return a[0] == 0xA5 && a[31] == 0x5A;
}
