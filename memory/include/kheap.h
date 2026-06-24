/*
 * AcharyaOS - kheap.h
 * -------------------
 * Phase 1, Feature 6: early kernel heap allocator. This is a deliberately
 * small bump allocator used before AcharyaOS has virtual memory, processes,
 * or per-object lifetime tracking.
 */

#ifndef ACHARYAOS_KHEAP_H
#define ACHARYAOS_KHEAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uintptr_t heap_start;
    uintptr_t heap_current;
    uintptr_t heap_end;
    size_t bytes_used;
    size_t allocation_count;
} kheap_stats_t;

void kheap_init(void);
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *ptr);
void kheap_get_stats(kheap_stats_t *stats);
int kheap_self_test(void);

#endif /* ACHARYAOS_KHEAP_H */
