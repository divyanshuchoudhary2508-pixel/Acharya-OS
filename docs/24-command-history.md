# AcharyaOS Phase 1, Feature 24 - Command History

## What this subsystem does

Feature 24 adds shell command history. The shell records executed commands
so they can be reviewed and recalled later.

## Dependencies

This subsystem depends on:

- the shell line editor
- keyboard input
- the kernel string helpers

## Architecture

Command history is kept in a small ring buffer inside the shell. Each
executed command is copied into the buffer unless it is a history-management
command itself.

Supported interactions:

- `history` to list stored commands
- `!!` to repeat the last command
- `!n` to repeat history entry `n`
- `clearhist` to clear the buffer

## Folder structure

```text
kernel/shell/shell.c
docs/24-command-history.md
```

## Debugging guide

If a command does not appear in history, confirm it was not a history
management command itself.

If recall fails, check that the requested history index is still inside the
ring buffer window.
