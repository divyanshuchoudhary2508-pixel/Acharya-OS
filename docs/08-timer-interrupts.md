# Phase 1, Feature 8: Timer Interrupts

## What It Does

Timer interrupts give AcharyaOS a steady heartbeat. The scheduler will later
use this heartbeat to decide when to switch tasks. For now, the timer driver
programs the PIT to fire around 100 times per second and increments a global
tick counter on every IRQ0 interrupt.

## Dependencies

- IDT: vector 32 receives IRQ0 after PIC remapping.
- PIC: IRQ0 must be unmasked and acknowledged with EOI.
- Port I/O: programs PIT ports `0x43` and `0x40`.
- Shell: exposes `ticks` for visible verification.

## Architecture

Files:

```text
drivers/include/timer.h
drivers/timer/timer.c
docs/08-timer-interrupts.md
```

Hardware flow:

```text
PIT channel 0 -> IRQ0 -> PIC -> IDT vector 32 -> timer_interrupt_handler()
```

The handler does intentionally little work:

1. Increment `ticks`.
2. Send PIC EOI.
3. Return with `iretq`.

That fast handler discipline is important because interrupts happen often.

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
ticks
ticks
```

Expected behavior:

- Kernel prints `[init] Starting PIT timer at 100 Hz... done.`
- Repeated `ticks` commands show an increasing tick count.
- The shell remains responsive while the timer is firing.

## Debugging Strategy

- If `ticks` stays at zero, confirm `timer_init()` registers vector 32 and
  unmasks IRQ0.
- If the first tick arrives but no more follow, suspect a missing PIC EOI.
- If keyboard input becomes sluggish, make sure the timer handler is still
  tiny and does not print from inside the interrupt.
- If QEMU resets, verify the IDT vector 32 stub and interrupt stack frame.
