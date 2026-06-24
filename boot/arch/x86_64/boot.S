/*
 * AcharyaOS - boot.S
 * ------------------
 * This is the very first AcharyaOS-owned code that runs.
 *
 * WHERE WE START:
 *   GRUB has already done the boring, fragile, hardware-specific work for us:
 *     - Loaded our kernel binary off disk into memory
 *     - Enabled the A20 line (lets us address >1MB of memory)
 *     - Switched the CPU from 16-bit real mode into 32-bit PROTECTED mode
 *     - Set up a basic GDT (Global Descriptor Table) of its own
 *     - Parsed the Multiboot2 header below so it knows how to load us
 *     - Jumped to the symbol "_start" (defined below) in 32-bit mode
 *
 * WHERE WE NEED TO GET TO:
 *   64-bit LONG mode, paging enabled, stack set up, and then a call into
 *   kmain() (our C kernel entry point) that never returns.
 *
 * WHY THIS IS HARD:
 *   x86_64 CPUs do not have a single instruction "switch to 64-bit mode."
 *   You must go through an exact, documented sequence:
 *     1. Build page tables (long mode REQUIRES paging - no exceptions)
 *     2. Enable PAE (Physical Address Extension) in CR4
 *     3. Load CR3 with the address of your top-level page table
 *     4. Set the LME (Long Mode Enable) bit in the EFER MSR
 *     5. Enable paging via the PG bit in CR0
 *        -> At this exact point the CPU is in "compatibility mode":
 *           32-bit instructions still work, but long mode is "armed."
 *     6. Load a GDT that has a proper 64-bit code segment
 *     7. Far jump into that 64-bit code segment
 *        -> THIS is the instant we are actually running 64-bit code.
 *
 *   Skipping or misordering any step = triple fault = instant silent reboot.
 *   This is why bootloaders are debugged with QEMU + serial logging, not guesswork.
 */

/* ============================================================
 * SECTION: .multiboot2
 * ============================================================
 * The Multiboot2 header is a magic-number-prefixed data structure that GRUB
 * scans for in the first 32KB of our kernel binary. It tells GRUB:
 *   "Yes, I am a Multiboot2-compliant kernel, here is how to load me."
 * It is pure DATA, not code - the CPU never executes this; GRUB *reads* it.
 */
.section .multiboot2,"a"
.align 8
multiboot_header:
    .long 0xe85250d6                /* Multiboot2 magic number (fixed, mandatory) */
    .long 0                          /* Architecture: 0 = i386 protected mode */
    .long multiboot_header_end - multiboot_header   /* Total header length */
    /* Checksum: magic + arch + length + checksum must sum to 0 (mod 2^32) */
    .long -(0xe85250d6 + 0 + (multiboot_header_end - multiboot_header))

    /* --- Tags: extra requests/info for GRUB. We keep this minimal for now. --- */

    /* End tag: marks the end of the tag list. Every Multiboot2 header needs this. */
    .align 8
    .word 0    /* type = 0 (end tag) */
    .word 0    /* flags */
    .long 8    /* size of this tag */
multiboot_header_end:


/* ============================================================
 * SECTION: .bss
 * ============================================================
 * Uninitialized data: our stack, and the page tables we build by hand.
 * .bss is zero-filled by the loader, so we don't waste disk space storing zeros.
 *
 * PAGE TABLE STRATEGY (kept deliberately simple for this first boot):
 *   We identity-map the first 1 GiB of physical memory (vaddr == paddr).
 *   That is: virtual address 0x0 -> physical address 0x0, and so on.
 *   This is NOT how a real kernel's final memory map looks (see memory/ subsystem
 *   later, which will set up a proper higher-half kernel mapping), but for the
 *   bootloader's only job - "get into long mode and reach kmain() alive" - an
 *   identity map is the simplest correct thing that works, and the architecture
 *   doc explains exactly why we'll change this later.
 *
 *   x86_64 4-level paging structures involved here:
 *     PML4 (Page Map Level 4) -> PDPT (Page Directory Pointer Table)
 *       -> PD (Page Directory) using 2MiB pages (so we skip the lowest PT level)
 *   Using 2MiB pages means ONE PD with 512 entries covers exactly 1 GiB.
 *   That's why we don't need a 4th-level Page Table array at all yet.
 */
.section .bss
.align 4096
pml4_table:
    .skip 4096
pdpt_table:
    .skip 4096
pd_table:
    .skip 4096

.align 16
stack_bottom:
    .skip 16384        /* 16 KiB boot stack - plenty for our needs before kmain() */
stack_top:


/* ============================================================
 * SECTION: .data
 * ============================================================
 * The 64-bit GDT we will load right before jumping into long mode.
 * In long mode, segmentation is almost entirely disabled - base/limit are
 * ignored for code/data segments - but the CPU still requires a valid GDT
 * with a code segment descriptor that has the "Long mode" (L) bit set,
 * because that L bit is literally what tells the CPU "treat this segment's
 * code as 64-bit instructions."
 */
.section .data
.align 16
gdt64:
    .quad 0                          /* Mandatory null descriptor (index 0) */
.equ gdt64.code, . - gdt64
    /* 64-bit code segment descriptor:
       - Present (bit 47), Descriptor type=1 code/data (bit 43),
         Executable (bit 43->44 region), Long mode (bit 53) */
    .quad 0x00AF9A000000FFFF
.equ gdt64.data, . - gdt64
    /* 64-bit data segment descriptor (writable, present) */
    .quad 0x00CF92000000FFFF
gdt64_end:

gdt64_pointer:
    .word gdt64_end - gdt64 - 1      /* GDT limit = size - 1 */
    .quad gdt64                      /* GDT base address */


/* ============================================================
 * SECTION: .text  (32-bit code - GRUB hands us control in protected mode)
 * ============================================================
 */
.section .text
.code32
.global _start
.type _start, @function
_start:
    mov $0x3F8 + 1, %dx
    xor %al, %al
    out %al, %dx
    mov $0x3F8 + 3, %dx
    mov $0x80, %al
    out %al, %dx
    mov $0x3F8 + 0, %dx
    mov $0x03, %al
    out %al, %dx
    mov $0x3F8 + 1, %dx
    xor %al, %al
    out %al, %dx
    mov $0x3F8 + 3, %dx
    mov $0x03, %al
    out %al, %dx
    movb $'S', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx
    /*
     * GRUB has passed us two values per the Multiboot2 spec:
     *   EAX = magic number 0x36d76289 (proves a Multiboot2-compliant loader booted us)
     *   EBX = physical address of the Multiboot2 information structure
     * We preserve EBX now because the memory subsystem will eventually want
     * that info (memory map, etc). For this first boot we just stash it.
     */
    mov %ebx, %edi          /* stash multiboot info pointer (callee-saved-ish for us) */

    /* Set up our own stack immediately - never trust the stack GRUB left us. */
    mov $stack_top, %esp
    movb $'s', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx

    /* --------------------------------------------------------
     * STEP 1: Build the page tables (identity-map first 1 GiB)
     * -------------------------------------------------------- */

    /* PML4[0] -> points to our PDPT. Flags: Present (bit0) + Writable (bit1) */
    mov $pdpt_table, %eax
    or  $0x3, %eax
    mov %eax, (pml4_table)

    /* PDPT[0] -> points to our PD. Same flags. */
    mov $pd_table, %eax
    or  $0x3, %eax
    mov %eax, (pdpt_table)

    /*
     * Fill all 512 entries of the Page Directory.
     * Each entry maps a 2MiB page, flags: Present(0) + Writable(1) + PageSize/Huge(7)
     * Entry N maps physical address N * 2MiB.
     * 512 entries * 2MiB = 1 GiB total mapped - exactly our boot-time identity map.
     */
    mov $0, %ecx                /* ecx = entry index, 0..511 */
fill_pd_table:
    mov $0x200000, %eax         /* 2 MiB */
    mul %ecx                    /* eax = 2MiB * ecx -> this entry's physical base */
    or  $0x83, %eax             /* Present + Writable + PageSize(huge page) bits */
    mov %eax, pd_table(,%ecx,8) /* each entry is 8 bytes; write low 32 bits */
    /* (high 32 bits of each 8-byte entry are already zero from .bss, which is
        correct since we're only mapping addresses below 4GiB here) */

    inc %ecx
    cmp $512, %ecx
    jne fill_pd_table

    /* --------------------------------------------------------
     * STEP 2: Enable PAE (Physical Address Extension) - mandatory for long mode
     * -------------------------------------------------------- */
    mov %cr4, %eax
    or  $(1 << 5), %eax          /* CR4.PAE = bit 5 */
    mov %eax, %cr4
    movb $'P', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx

    /* --------------------------------------------------------
     * STEP 3: Load CR3 with our PML4 table's physical address
     * -------------------------------------------------------- */
    mov $pml4_table, %eax
    mov %eax, %cr3
    movb $'3', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx

    /* --------------------------------------------------------
     * STEP 4: Set the Long Mode Enable (LME) bit in the EFER MSR
     * -------------------------------------------------------- */
    mov $0xC0000080, %ecx        /* EFER MSR number */
    rdmsr                        /* reads MSR[ecx] into edx:eax */
    or  $(1 << 8), %eax          /* EFER.LME = bit 8 */
    wrmsr
    movb $'L', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx

    /* --------------------------------------------------------
     * STEP 5: Enable paging (CR0.PG) - this "arms" long mode.
     *         We are now in IA-32e "compatibility submode" until we
     *         actually load a 64-bit code segment via far jump.
     * -------------------------------------------------------- */
    mov %cr0, %eax
    or  $(1 << 31), %eax         /* CR0.PG = bit 31 */
    mov %eax, %cr0
    movb $'G', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx

    /* --------------------------------------------------------
     * STEP 6: Load our 64-bit GDT
     * -------------------------------------------------------- */
    lgdt gdt64_pointer
    movb $'D', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx

    /* --------------------------------------------------------
     * STEP 7: Far jump into the 64-bit code segment.
     *         This single instruction is the moment we truly become
     *         a 64-bit kernel. Everything after this label is .code64.
     * -------------------------------------------------------- */
    ljmp $gdt64.code, $long_mode_entry


/* ============================================================
 * 64-bit code from here on
 * ============================================================ */
.code64
long_mode_entry:
    movb $'6', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx
    /* Reload data segment registers with our 64-bit data selector.
       In long mode these mostly don't matter semantically, but the CPU
       still wants valid selectors loaded - leaving stale 32-bit selectors
       in place is a classic source of confusing triple-faults. */
    mov $gdt64.data, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    /* Re-establish the stack pointer as a full 64-bit value. */
    mov $stack_top, %rsp

    /*
     * Pass the Multiboot2 info pointer to kmain() as its first argument.
     * x86_64 System V calling convention: 1st integer arg goes in RDI.
     * We stashed it in EDI earlier; zero-extend it into RDI explicitly
     * since we're crossing from 32-bit to 64-bit code.
     */
    mov %edi, %edi    /* zero-extends edi into the full 64-bit rdi (documented x86_64 behavior) */

    /* Call into our C kernel. This call should never return. */
    movb $'K', %al
    outb %al, $0xE9
    mov $0x3F8, %dx
    out %al, %dx
    call kmain

    /*
     * Safety net: if kmain() ever DOES return (it shouldn't), halt forever
     * rather than fall off into garbage memory and execute random bytes.
     */
hang:
    cli            /* disable interrupts */
    hlt            /* halt the CPU */
    jmp hang       /* in case of NMI waking us up, halt again */
