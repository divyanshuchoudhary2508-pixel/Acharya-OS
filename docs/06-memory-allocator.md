# Phase 1, Feature 6: Memory Allocator

## What It Does

The memory allocator gives kernel code its first dynamic allocation API:

- `kmalloc(size)`
- `kzalloc(size)`
- `kfree(ptr)`
- `kheap_get_stats(stats)`

This first allocator is an early bump allocator. It hands out aligned memory
from a fixed 1 MiB region immediately after the kernel image.

## Dependencies

- Bootloader identity-maps the first 1 GiB, so memory just after the kernel is
  accessible.
- Linker script exports `__kernel_end`, the first page-aligned address after
  kernel code/data/BSS.
- `kstring` provides `memset()` for `kzalloc()`.
- Shell provides the `mem` command for live inspection.

## Architecture

The allocator maintains three addresses:

- `heap_start`: first usable heap byte.
- `heap_current`: next allocation cursor.
- `heap_end`: fixed limit, currently `heap_start + 1 MiB`.

Each allocation is aligned to 16 bytes. `kzalloc()` calls `kmalloc()` and clears
the result. `kfree()` is intentionally a no-op for now.

This is not the final heap design. It is the correct educational stepping stone
before Virtual Memory, because AcharyaOS does not yet have page frame ownership,
processes, or enough lifetime information to safely reclaim memory.

## Files

```text
memory/include/kheap.h
memory/kheap.c
docs/06-memory-allocator.md
```

Supporting changes:

```text
boot/arch/x86_64/linker.ld   exports __kernel_end
Makefile                     includes memory/ sources
kernel/kmain.c               initializes and self-tests heap
kernel/shell/shell.c         adds mem command
```

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
mem
echo allocator works
help
```

Expected behavior:

- Kernel prints `[init] Starting early kernel heap... done.`
- Shell starts normally.
- `mem` prints heap start/current/end, bytes used, and allocation count.
- `bytes used` is nonzero because the boot self-test performs allocations.

## Debugging Strategy

- If heap init fails early, inspect the linker map and confirm
  `__kernel_end` exists and is page-aligned.
- If allocations return null immediately, check whether the heap start/end
  calculation overflowed or the heap size is too small.
- If `kzalloc()` returns nonzero memory, check `memset()` and the self-test.
- If later subsystems appear to corrupt memory, add temporary guard writes
  around allocations and inspect heap usage with the shell `mem` command.
