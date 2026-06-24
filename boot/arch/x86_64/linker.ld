/*
 * AcharyaOS - linker.ld
 * ----------------------
 * A linker script tells the linker (ld) WHERE in memory each piece of our
 * compiled code and data should end up living, and in what order.
 *
 * Without this, the linker uses a generic default layout meant for normal
 * userspace programs running under an OS - which assumes things (like a
 * dynamic loader, shared libraries, an OS placing us at some virtual address)
 * that simply don't exist yet. We have to tell it the truth: this is a
 * freestanding kernel binary that GRUB will load at a fixed physical address.
 *
 * MEMORY LAYOUT DECISION: we load the kernel at 1 MiB (0x100000).
 *   Why 1 MiB? Physical memory below 1 MiB is a minefield of legacy reserved
 *   regions (BIOS data area, video memory, option ROMs). 1 MiB is the
 *   traditional, safe, universally-supported load address for Multiboot
 *   kernels - GRUB expects to be able to put us there.
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    /*
     * The Multiboot2 header MUST be within the first 32KB of the file AND
     * placed before any other code, so we put it in its own section first.
     */
    .multiboot2 : ALIGN(8)
    {
        KEEP(*(.multiboot2))
    }

    .text : ALIGN(4096)
    {
        *(.text)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4096);
    __kernel_end = .;
}
