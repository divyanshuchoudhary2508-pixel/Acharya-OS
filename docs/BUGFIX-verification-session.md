# AcharyaOS - Real Boot Verification & Bug Fix Session

This documents a verification pass performed with a real x86_64 toolchain
(gcc, binutils, grub-mkrescue, xorriso, qemu-system-x86_64) and actual QEMU
boot testing - not syntax-only checking. Every fix below was confirmed by
rebuilding and re-booting, with keyboard input sent through QEMU's real
emulated PS/2 device and output read from the serial console.

## Bug 1: `.multiboot2` section missing the allocatable flag
**File:** `boot/arch/x86_64/boot.S`
**Present since:** Feature 1
**Symptom:** Later features produced an empty serial log on boot.

**Root cause:** `.section .multiboot2` had no explicit section flags, so
GNU `as` did not mark it allocatable. The linker excluded it from the
LOAD segment and GRUB could no longer reliably find the Multiboot2 header
inside the first 32KB scan window.

**Fix:** `.section .multiboot2, "a"`

## Bug 2: invalid operand size in `userspace_enter` inline asm
**File:** `users/userspace.c`
**Symptom:** Compile error under a real x86_64 toolchain.

**Root cause:** a 64-bit operand was moved directly into `%ax`.

**Fix:** use a 16-bit-sized value for the segment selector.

## Bug 3: missing `SS` push in the `iretq` frame
**File:** `users/userspace.c`
**Symptom:** Immediate fault on ring-3 transition.

**Root cause:** `iretq` pops RIP, CS, RFLAGS, RSP, and SS, but the frame
only pushed four values.

**Fix:** add the missing `SS` push.

## Bug 4: missing PDPT-level page table entry
**File:** `memory/vmm.c`
**Symptom:** User mode jumped to the correct address and then page-faulted
immediately on the first instruction fetch.

**Root cause:** the PDPT-to-PD link was never written.

**Fix:** write the missing PDPT entry before mapping the leaf page.

## Bug 5: no TSS configured
**Files:** `kernel/arch/x86_64/gdt.c`, `gdt.h`, `gdt_flush.S`
**Symptom:** Ring transitions requiring a kernel stack stalled.

**Root cause:** x86_64 needs a valid TSS loaded via `ltr` for privilege
changes that land in kernel mode.

**Fix:** add a minimal TSS and load it during GDT setup.

## Bug 6 (minor): buffer size mismatch in `fs_write_label`
**File:** `fs/fs.c`
**Symptom:** compiler warning about a potential overflow.

**Fix:** align the buffer size and copy bounds with the real on-disk
label field.

## What this means

The fixes above were found by actually building and booting the kernel in
QEMU with a real x86_64 toolchain. That exposed bugs that syntax-only
checking would not catch.
