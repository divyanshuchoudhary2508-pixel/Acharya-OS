# AcharyaOS Phase 1, Feature 25 - Task Manager

## What this subsystem does

Feature 25 adds a terminal task manager. It gives a compact live view of the
kernel process table and scheduler state from inside the shell.

## Dependencies

This subsystem depends on:

- the process table
- the scheduler accounting layer
- the shell command dispatcher
- kernel text output

## Architecture

The task manager is a shell command, `taskmgr`, rather than a separate GUI
application. That keeps the feature lightweight and lets us reuse the data
already tracked by process management and scheduling.

The view combines:

- process IDs and names
- process states
- scheduler task IDs
- scheduler tick counts

This is intentionally read-only for now. It is an inspection tool, not a task
control surface.

## Folder structure

```text
kernel/shell/shell.c
docs/25-task-manager.md
```

## Debugging guide

If `taskmgr` shows no processes, check that `process_init()` ran and that the
kernel shell process was registered.

If scheduler counts stay at zero, check the timer interrupt path and confirm
that `scheduler_on_timer_tick()` is still being called.

If the command is missing, confirm the shell help text and command dispatch
table were updated together.
