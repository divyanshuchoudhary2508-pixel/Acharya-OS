# Phase 1, Feature 10: Process Management

## What It Does

Process Management introduces the kernel's process table. A process is the
kernel-owned record for an executable entity:

- PID
- name
- state
- creation tick
- scheduler task binding

This is still before user-space programs and system calls, so processes are
kernel metadata records rather than loaded ELF programs.

## Dependencies

- Scheduler: each created process registers a runnable scheduler task.
- Timer: process creation records the current tick.
- Shell: exposes `ps` and `spawn` for inspection.

## Architecture

Files:

```text
process/include/process.h
process/process.c
docs/10-process-management.md
```

Ownership split:

- Process Management owns PID allocation and process states.
- Scheduler owns time slices and round-robin accounting.
- Timer provides the heartbeat both systems observe.

Initial processes:

- `kernel-shell`
- `idle`

The `spawn` shell command creates demo kernel-process metadata. It does not
start executing a separate stack yet; that belongs to the next context-switching
step.

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
ps
sched
spawn
ps
sched
```

Expected behavior:

- Kernel prints `[init] Creating initial kernel processes... done.`
- `ps` shows `kernel-shell` and `idle`.
- `spawn` creates `demo-kernel-task` until the fixed process table is full.
- `sched` shows matching scheduler tasks and increasing tick counts.

## Debugging Strategy

- If `ps` is empty, confirm `process_init()` runs after `scheduler_init()`.
- If `spawn` fails immediately, check `PROCESS_MAX` and `SCHED_MAX_TASKS`.
- If a process exists but no scheduler task exists, inspect
  `scheduler_register_task()` return handling.
- If tick counts do not increase, debug Timer Interrupts before Process
  Management.
