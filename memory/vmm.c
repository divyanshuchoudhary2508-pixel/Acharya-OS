/*
 * AcharyaOS - vmm.c
 * -----------------
 * Kernel-owned paging setup. This is still an identity map, intentionally:
 * virtual address X maps to physical address X for the first 1 GiB. The value
 * of this feature is ownership and structure, not sophistication. Future
 * Virtual Memory work can add higher-half mappings, user pages, and page fault
 * handling without leaving boot.S as the permanent owner of paging.
 */

#include "vmm.h"
#include "kstring.h"

#define PAGE_ENTRIES 512
#define PAGE_PRESENT (1ull << 0)
#define PAGE_WRITABLE (1ull << 1)
#define PAGE_USER (1ull << 2)
#define PAGE_HUGE_2M (1ull << 7)

typedef uint64_t page_table_t[PAGE_ENTRIES];

static page_table_t kernel_pml4 __attribute__((aligned(4096)));
static page_table_t kernel_pdpt __attribute__((aligned(4096)));
static page_table_t kernel_pd __attribute__((aligned(4096)));
static page_table_t user_pdpt __attribute__((aligned(4096)));
static page_table_t user_pd __attribute__((aligned(4096)));

static size_t mapped_pages;

static inline void load_cr3(uintptr_t pml4_physical_address) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4_physical_address) : "memory");
}

static inline uintptr_t read_cr3(void) {
    uintptr_t value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

void vmm_init(void) {
    memset(kernel_pml4, 0, sizeof(kernel_pml4));
    memset(kernel_pdpt, 0, sizeof(kernel_pdpt));
    memset(kernel_pd, 0, sizeof(kernel_pd));
    memset(user_pdpt, 0, sizeof(user_pdpt));
    memset(user_pd, 0, sizeof(user_pd));

    kernel_pml4[0] = ((uintptr_t) kernel_pdpt) | PAGE_PRESENT | PAGE_WRITABLE;
    kernel_pdpt[0] = ((uintptr_t) kernel_pd) | PAGE_PRESENT | PAGE_WRITABLE;
    kernel_pml4[1] = ((uintptr_t) user_pdpt) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    mapped_pages = VMM_IDENTITY_MAP_SIZE / VMM_PAGE_SIZE_2M;
    for (size_t i = 0; i < mapped_pages; i++) {
        uintptr_t physical = (uintptr_t)(i * VMM_PAGE_SIZE_2M);
        kernel_pd[i] = physical | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE_2M;
    }

    load_cr3((uintptr_t) kernel_pml4);
}

int vmm_map_user_2m(uintptr_t virtual_address, uintptr_t physical_address) {
    if ((virtual_address % VMM_PAGE_SIZE_2M) != 0 || (physical_address % VMM_PAGE_SIZE_2M) != 0) {
        return -1;
    }

    if (virtual_address < VMM_IDENTITY_MAP_SIZE) {
        return -1;
    }

    uint64_t va = (uint64_t) virtual_address;
    uint64_t pml4_index = (va >> 39) & 0x1FFu;
    uint64_t pdpt_index = (va >> 30) & 0x1FFu;
    uint64_t pd_index = (va >> 21) & 0x1FFu;

    if (pml4_index != 1) {
        return -1;
    }

    if (pdpt_index != 0) {
        return -1;
    }

    if ((kernel_pml4[pml4_index] & PAGE_PRESENT) == 0) {
        kernel_pml4[pml4_index] = ((uintptr_t) user_pdpt) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    if ((user_pdpt[pdpt_index] & PAGE_PRESENT) == 0) {
        user_pdpt[pdpt_index] = ((uintptr_t) user_pd) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    user_pd[pd_index] = physical_address | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER | PAGE_HUGE_2M;
    return 0;
}

uintptr_t vmm_translate_identity(uintptr_t virtual_address) {
    if (virtual_address >= VMM_IDENTITY_MAP_SIZE) {
        return 0;
    }

    size_t pd_index = (virtual_address >> 21) & 0x1FFu;
    uint64_t entry = kernel_pd[pd_index];
    if ((entry & PAGE_PRESENT) == 0 || (entry & PAGE_HUGE_2M) == 0) {
        return 0;
    }

    uintptr_t physical_base = (uintptr_t)(entry & 0x000FFFFFFFFFF000ull);
    return physical_base + (virtual_address & (VMM_PAGE_SIZE_2M - 1u));
}

void vmm_get_stats(vmm_stats_t *stats) {
    if (!stats) {
        return;
    }

    stats->pml4_address = read_cr3();
    stats->mapped_start = 0;
    stats->mapped_end = VMM_IDENTITY_MAP_SIZE;
    stats->mapped_2m_pages = mapped_pages;
}

int vmm_self_test(void) {
    vmm_stats_t stats;
    vmm_get_stats(&stats);

    if (stats.pml4_address != (uintptr_t) kernel_pml4) {
        return 0;
    }
    if (vmm_translate_identity(0x100000u) != 0x100000u) {
        return 0;
    }
    if (vmm_translate_identity(0xB8000u) != 0xB8000u) {
        return 0;
    }
    if (vmm_translate_identity(VMM_IDENTITY_MAP_SIZE) != 0) {
        return 0;
    }

    return 1;
}
