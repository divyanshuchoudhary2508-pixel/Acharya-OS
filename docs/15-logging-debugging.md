# AcharyaOS Phase 1, Feature 15 - Logging and Debugging Framework

## What this subsystem does

This feature adds a small kernel logging service with:

- log levels
- a fixed-size in-memory ring buffer
- boot-time diagnostic messages
- shell commands to inspect and tune logging

The goal is to make kernel behavior easier to understand without relying
only on live console output.

## Dependencies

This subsystem depends on:

- `timer` for timestamps
- `kio` for console output
- `kstring` for bounded string handling
- `kernel` for panic integration

It does not depend on the filesystem, scheduler, or user space.

## Architecture

The logger keeps the last `LOG_BUFFER_MAX` messages in a ring buffer.
Each entry stores:

- timestamp tick
- log level
- message text

The public API is intentionally small:

- `log_init()`
- `log_write()`
- `log_writef()`
- `log_copy_entries()`
- `log_get_stats()`

The shell exposes two commands:

- `log` to print the buffered entries
- `loglevel` to inspect or change the minimum log level

## Folder structure

```text
kernel/include/log.h
kernel/debug/log.c
docs/15-logging-debugging.md
```

## Build notes

The Makefile discovers new `.c` files automatically, so no build-system
change is needed as long as the new source file lives under a tracked
source directory.

## Debugging guide

If logs do not appear:

1. Check that `log_init()` runs during kernel boot.
2. Use the `log` shell command to inspect the buffer.
3. Use `loglevel` to make sure the minimum log level is not filtering out
   the messages you expect.
4. Verify timer interrupts are running, because log timestamps come from
   the timer tick counter.

If the machine panics, the panic handler records the panic message in the
log buffer before halting.
