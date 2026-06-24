# Phase 1, Feature 7: Virtual Memory

## What It Does

Virtual Memory makes paging a kernel-owned subsystem. The bootloader still
creates the first temporary page tables because x86_64 long mode requires
paging before C code can run. This feature replaces those temporary tables with
page tables built and loaded by the kernel.

The current mapping is intentionally simple:

- Identity-map the first 1 GiB.
- Use 2 MiB pages.
- Keep kernel, VGA memory, heap, and early hardware ranges accessible.

## Dependencies

- Bootloader: enters long mode using temporary identity paging.
- Text Output: verifies VGA remains accessible after CR3 is switched.
- Memory Allocator: continues to work because the heap sits inside the first
  1 GiB identity map.
- Shell: exposes `vmem` for interactive inspection.

## Architecture

Files:

```text
memory/include/vmm.h
memory/vmm.c
docs/07-virtual-memory.md
```

The paging hierarchy is:

```text
PML4[0] -> PDPT[0] -> PD[0..511]
```

Each page-directory entry maps one 2 MiB page. 512 entries cover 1 GiB.

This is not the final virtual address layout. A later revision can add a
higher-half kernel, page fault handler, user/kernel permissions, and demand
allocation. The important milestone here is that paging state is now described
in C and owned by the kernel instead of being hidden in `boot.S`.

## Build

```sh
make clean
make
make iso
```

## QEMU Test Plan

```sh
make run
```

Then type:

```text
vmem
mem
echo paging survived
```

Expected behavior:

- Kernel prints `[init] Installing kernel-owned page tables... done.`
- VGA output continues after CR3 is loaded.
- Shell input continues after interrupts are enabled.
- `vmem` shows an active PML4 address, mapped range `0x0` to `0x40000000`,
  512 2 MiB pages, and VGA translation `0xb8000`.

## Debugging Strategy

- If QEMU resets at virtual memory init, the new page tables probably do not
  identity-map the currently executing kernel code or stack.
- If output stops immediately after loading CR3, confirm `0xB8000` remains
  mapped.
- If the shell dies later, confirm the heap range after `__kernel_end` is still
  below the 1 GiB identity-map limit.
- If `vmem` prints zero for VGA translation, inspect the 2 MiB page directory
  entry covering physical address `0x000B8000`.
