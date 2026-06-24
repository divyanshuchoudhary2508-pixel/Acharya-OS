# Phase 1, Feature 11: System Calls

## What It Does

System calls provide the controlled entry point from future user-space programs
into kernel services. AcharyaOS uses `int 0x80` for this first version because
it is explicit, portable across the project's current interrupt machinery, and
easy to reason about in a teaching kernel.

## Dependencies

- IDT: vector 128 must be installed as a user-callable interrupt gate.
- Process management: syscall handlers query current PID and process count.
- Timer: provides a system-wide uptime counter.
- Text output: `SYSCALL_PUTS` writes to the kernel console for early testing.

## Architecture

Files:

```text
syscall/include/syscall.h
syscall/syscall.c
docs/11-system-calls.md
```

Syscall numbers:

- `0` `SYSCALL_PUTS`
- `1` `SYSCALL_GET_TICKS`
- `2` `SYSCALL_GET_PID`
- `3` `SYSCALL_GET_PROCESS_COUNT`
- `4` `SYSCALL_YIELD`

The kernel installs an `int 0x80` gate with ring-3 privilege so future
user-space code can invoke it. For now, the shell uses direct wrappers to prove
the path works end to end.

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
syscall
ps
ticks
```

Expected behavior:

- Kernel prints `[init] Installing syscall interface... done.`
- `syscall` prints a line via `int 0x80`.
- `syscall getpid` returns the current kernel-shell PID.
- `syscall get_ticks` matches the timer counter.
- `syscall process_count` matches the process table.

## Debugging Strategy

- If `int 0x80` triple-faults, confirm vector 128 is installed with DPL 3.
- If syscall return values are wrong, inspect how the interrupt frame maps
  saved registers and whether `frame->rax` is being updated by the handler.
- If the kernel can call wrappers but future user-space cannot, re-check the
  gate privilege bits and CS selector.
- If `syscall` prints nothing, inspect `SYSCALL_PUTS` and `kputs()`.
