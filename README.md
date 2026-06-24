# AcharyaOS

AcharyaOS is a hobby operating system built from scratch for x86_64.
It is written primarily in C with a small amount of x86_64 assembly, and it
uses a simple Make-based build flow.

## Build Requirements

- `x86_64-elf-gcc`
- `x86_64-elf-ld`
- `grub-mkrescue`
- `qemu-system-x86_64`

The kernel build is intended for a real x86_64 cross-compiler. Native
32-bit MinGW GCC is not suitable for building the kernel.

## What is in the repo

- bootloader and 64-bit kernel bring-up
- text output, keyboard input, and shell
- memory management, scheduler, process table, and syscalls
- disk I/O and a simple filesystem
- user-space demo support
- logging, configuration, package, service, and user account features
- framebuffer graphics, window manager, mouse support, and buttons

## Layout

```text
boot/       Boot and linker pieces
kernel/     Core kernel subsystems and shell
drivers/    Hardware-facing drivers
memory/     Heap and virtual memory
process/    Process metadata
scheduler/  Round-robin scheduler
syscall/    System call entry points
fs/         Simple filesystem
gui/        Framebuffer UI, windows, and controls
users/      User-space demo program
tools/      GRUB config and helper files
docs/       Feature-by-feature documentation
tests/      Test and verification notes
```

## Build

The repository is organized around the local Makefile in this tree.

```bash
make
```

If the cross-toolchain is missing, the build stops early and prints the
commands that need to be installed.

## Status

This is an incremental educational OS project. The codebase is intentionally
small, modular, and documented feature by feature.
