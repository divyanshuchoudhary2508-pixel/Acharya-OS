# Phase 1, Feature 14: User-Space Programs

## What It Does

User-space programs introduce ring-3 execution. The kernel now prepares a small
user code page and user stack, maps them with user permissions, and transfers
control to a demo program using `iretq`.

This is the first real privilege boundary in AcharyaOS.

## Dependencies

- Virtual Memory: must be able to map user-accessible pages.
- System Calls: the demo program uses `int 0x80`.
- Process Management: user execution will eventually grow into process-owned
  address spaces and lifetimes.

## Architecture

Files:

```text
users/include/userspace.h
users/userspace.c
users/demo_user.S
docs/14-user-space-programs.md
```

Current demo model:

- copy a prebuilt user program blob into a user-mapped page
- map a user stack page
- transition with `iretq` to ring 3
- use the existing syscall path for output and diagnostics

The current demo is intentionally tiny. It proves privilege separation without
requiring a full ELF loader or a complete process image format yet.

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
userdemo
```

Expected behavior:

- Kernel prints `[init] Preparing user-space demo... done.`
- Shell command `userdemo` enters ring 3.
- The user program prints `hello from user mode`.
- The program keeps running in user space and repeatedly yields.

## Debugging Strategy

- If the machine triple-faults on transition, inspect the user code/data
  selectors and the `iretq` frame.
- If the user program faults immediately, confirm the user code and stack
  pages are mapped with user permissions.
- If `int 0x80` fails from user mode, confirm the syscall gate is ring-3
  callable.
- If the demo never reaches user mode, inspect the copy into the user code
  page and the entry address passed to `iretq`.
