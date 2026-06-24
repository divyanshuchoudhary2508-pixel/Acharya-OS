/*
 * AcharyaOS - gdt.c
 * ------------------
 * Builds a permanent 5-entry GDT (null, kernel code, kernel data, user
 * code, user data) as a real C struct array, then asks gdt_flush() (in
 * gdt.S) to load it via `lgdt` and reload the segment registers.
 *
 * We include user-mode (ring 3) segments NOW, even though nothing uses
 * them yet, because:
 *   - The GDT layout is exactly the kind of thing other subsystems
 *     (Syscalls, User-Space Programs) will need to reference by a fixed
 *     selector index. Settling the layout once, here, avoids a breaking
 *     change later.
 *   - It costs us nothing: two unused-for-now descriptors, clearly
 *     commented as such.
 * This is a deliberate exception to "don't write code nothing calls yet" -
 * the DATA here (the GDT layout) is structural and load-bearing for later
 * subsystems in a way a speculative *function* wouldn't be.
 */

#include "gdt.h"
#include "kernel.h"

/*
 * GDT entry format (8 bytes), per the x86_64 architecture manual.
 * In 64-bit mode, base/limit are almost entirely IGNORED by the CPU for
 * code/data segments (paging does the real address translation work) -
 * but the byte LAYOUT below is still mandatory; the CPU parses these exact
 * fields out of the descriptor regardless of whether it uses the values.
 */
typedef struct PACKED {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;       /* Present, Privilege(2 bits), Type, Executable, etc */
    uint8_t  granularity;  /* Limit high nibble + flags (Long-mode bit lives here) */
    uint8_t  base_high;
} gdt_entry_t;

typedef struct PACKED {
    uint16_t limit;
    uint64_t base;
} gdt_pointer_t;

/* Segment selector indices - other subsystems (syscalls, user processes)
   will reference these by name rather than hardcoding "0x08" etc. */
enum {
    GDT_SEGMENT_NULL        = 0,
    GDT_SEGMENT_KERNEL_CODE = 1,
    GDT_SEGMENT_KERNEL_DATA = 2,
    GDT_SEGMENT_USER_CODE   = 3,  /* unused until User-Space Programs feature */
    GDT_SEGMENT_USER_DATA   = 4,  /* unused until User-Space Programs feature */
    GDT_SEGMENT_COUNT       = 5
};

static gdt_entry_t   gdt_entries[GDT_SEGMENT_COUNT];
static gdt_pointer_t gdt_pointer;

/* Implemented in gdt_flush.S - actually executes `lgdt` and reloads CS/SS/etc.
   This MUST be assembly: there is no way to express "reload the CS
   register via a far jump-like mechanism" in portable C. */
extern void gdt_flush(uint64_t gdt_pointer_address);

/*
 * gdt_set_entry: fill one descriptor. Parameters mirror the real hardware
 * fields directly rather than being "friendly" wrappers, because the goal
 * here is for the reader to see exactly what the CPU sees - this is a
 * teaching-oriented kernel, and a clever abstraction here would hide the
 * exact mechanism the project's docs are supposed to explain.
 */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t granularity) {
    gdt_entries[index].base_low    = (uint16_t)(base & 0xFFFF);
    gdt_entries[index].base_mid    = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[index].base_high   = (uint8_t)((base >> 24) & 0xFF);
    gdt_entries[index].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt_entries[index].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (granularity & 0xF0));
    gdt_entries[index].access      = access;
}

void gdt_init(void) {
    /* Entry 0: the mandatory null descriptor. The CPU requires GDT index 0
       to be all zeros; loading the null selector is how you intentionally
       mark a segment register "unused." */
    gdt_set_entry(GDT_SEGMENT_NULL, 0, 0, 0, 0);

    /*
     * Entry 1: kernel code segment.
     * access = 0x9A = Present(1) DPL=00(ring0) S=1(code/data) Type=1010(exec,read)
     * granularity = 0xAF = Long-mode bit(1) + Limit-is-4K-granular(1) + top limit nibble(F)
     * In 64-bit mode base/limit values themselves are ignored by the CPU,
     * but we set them to the conventional "whole address space" values
     * (0, 0xFFFFF) for clarity and in case any tooling inspects them.
     */
    gdt_set_entry(GDT_SEGMENT_KERNEL_CODE, 0, 0xFFFFF, 0x9A, 0xAF);

    /* Entry 2: kernel data segment.
       access = 0x92 = Present(1) DPL=00(ring0) S=1 Type=0010(read,write) */
    gdt_set_entry(GDT_SEGMENT_KERNEL_DATA, 0, 0xFFFFF, 0x92, 0xCF);

    /* Entry 3: user code segment (ring 3). DPL=11 -> access bits 5-6 set.
       0xFA = Present(1) DPL=11 S=1 Type=1010(exec,read) */
    gdt_set_entry(GDT_SEGMENT_USER_CODE, 0, 0xFFFFF, 0xFA, 0xAF);

    /* Entry 4: user data segment (ring 3).
       0xF2 = Present(1) DPL=11 S=1 Type=0010(read,write) */
    gdt_set_entry(GDT_SEGMENT_USER_DATA, 0, 0xFFFFF, 0xF2, 0xCF);

    gdt_pointer.limit = (uint16_t)(sizeof(gdt_entries) - 1);
    gdt_pointer.base  = (uint64_t) &gdt_entries;

    gdt_flush((uint64_t) &gdt_pointer);
}
