/*
 * AcharyaOS - vmm.h
 * -----------------
 * Phase 1, Feature 7: Virtual Memory manager. Paging is already enabled by
 * boot.S so long mode can exist; this subsystem takes ownership of the page
 * tables from C and exposes simple mapping/inspection helpers.
 */

#ifndef ACHARYAOS_VMM_H
#define ACHARYAOS_VMM_H

#include <stddef.h>
#include <stdint.h>

#define VMM_PAGE_SIZE_2M 0x200000ull
#define VMM_IDENTITY_MAP_SIZE (1024ull * 1024ull * 1024ull)

typedef struct {
    uintptr_t pml4_address;
    uintptr_t mapped_start;
    uintptr_t mapped_end;
    size_t mapped_2m_pages;
} vmm_stats_t;

void vmm_init(void);
uintptr_t vmm_translate_identity(uintptr_t virtual_address);
void vmm_get_stats(vmm_stats_t *stats);
int vmm_self_test(void);
int vmm_map_user_2m(uintptr_t virtual_address, uintptr_t physical_address);

#endif /* ACHARYAOS_VMM_H */
