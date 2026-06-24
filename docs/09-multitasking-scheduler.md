# Phase 1, Feature 9: Multitasking Scheduler

## What It Does

The scheduler decides which runnable task should receive CPU time. In this
milestone, AcharyaOS adds a round-robin scheduler core that advances on every
timer interrupt and tracks per-task time accounting.

This is the policy layer. It does not yet perform CPU context switching. That
belongs to Feature 10, Process Management, where tasks gain saved register
contexts, kernel stacks, and ownership/lifetime rules.

## Dependencies

- Timer Interrupts: `scheduler_on_timer_tick()` is called from IRQ0.
- Shell: exposes `sched` for inspection.
- Memory Allocator and Virtual Memory: provide the foundation for later task
  stacks and process-owned memory.

## Architecture

Files:

```text
scheduler/include/scheduler.h
scheduler/scheduler.c
docs/09-multitasking-scheduler.md
```

The scheduler maintains a small fixed table:

- task id
- task name
- state
- time slice length
- ticks charged to that task

Every PIT tick:

1. Increment global scheduler tick count.
2. Charge the current runnable task.
3. Advance to the next runnable task when its time slice expires.

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
sched
ticks
sched
```

Expected behavior:

- Kernel prints `[init] Starting round-robin scheduler core... done.`
- Shell reports Feature 9.
- `sched` shows at least `kernel-shell` and `idle`.
- Repeated `sched` commands show increasing task tick counts.

## Debugging Strategy

- If scheduler ticks stay at zero but `ticks` increases, confirm
  `timer.c` calls `scheduler_on_timer_tick()`.
- If both scheduler ticks and timer ticks stay zero, debug PIT/PIC/IDT first.
- If the current task id never changes, inspect each task's time slice and
  `advance_to_next_runnable()`.
- If QEMU resets after enabling interrupts, inspect timer vector 32 and the
  interrupt stack frame before blaming scheduler policy.
