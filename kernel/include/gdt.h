/*
 * AcharyaOS - gdt.h
 * ------------------
 * WHY we're rebuilding the GDT here, when boot.S already built one:
 *
 * boot.S's GDT exists for exactly one purpose: surviving the far jump into
 * long mode. It is two segments (kernel code + kernel data) hand-encoded as
 * raw hex in assembly, which is correct but completely opaque - you can't
 * inspect, extend, or reason about it from C, and it has no user-mode
 * segments at all.
 *
 * The KERNEL needs a permanent, more complete GDT because:
 *   1. Phase 1's "User-Space Programs" feature will need ring-3 (user mode)
 *      code/data segments, which boot.S's minimal GDT doesn't have.
 *   2. Expressing the GDT as a C struct array means future subsystems can
 *      read/modify it in a typed, checkable way instead of hand-editing
 *      raw hex constants.
 *   3. This is the natural permanent home for the GDT - boot.S's version
 *      was always meant to be transitional scaffolding, not the final
 *      answer (this is explicitly called out in the bootloader's own
 *      documentation as a known, intentional limitation).
 *
 * We are NOT removing boot.S's GDT - it still needs to exist for the
 * mode transition to work before kmain() is even reachable. This module's
 * gdt_init() is called early in kmain() and REPLACES the bootloader's GDT
 * with this permanent one via a fresh `lgdt` + segment reload, the same
 * mechanism boot.S itself used.
 */

#ifndef ACHARYAOS_GDT_H
#define ACHARYAOS_GDT_H

void gdt_init(void);
void gdt_load_tss(void);

#endif /* ACHARYAOS_GDT_H */
