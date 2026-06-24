# AcharyaOS Phase 1, Feature 18 - File Browser, Terminal Mode

## What this subsystem does

Feature 18 adds a terminal-mode file browser that helps inspect the simple
filesystem from inside the shell.

It can:

- list files
- filter by name prefix
- preview file contents
- show file metadata

## Dependencies

This subsystem depends on:

- the shell, to launch the browser
- `fs`, to enumerate and read files
- keyboard input, for browser commands
- `kio` and `vga`, for terminal output
- `kstring`, for small string operations

## Architecture

The filesystem is still flat, so the browser is also intentionally flat.
Rather than pretending there are directories, the browser works with:

- a current filter prefix
- a file list view
- preview and metadata actions

This keeps the feature honest to the current storage model.

## Folder structure

```text
kernel/include/browser.h
kernel/browser/browser.c
docs/18-file-browser-terminal-mode.md
```

## Debugging guide

If the browser cannot start:

1. Check that the filesystem is mounted.
2. Confirm the shell command is `browse`.
3. Make sure keyboard input still works in the shell.

If a file does not open:

1. Verify the file exists with `ls` in the browser or `ls` in the shell.
2. Check the file name spelling.
3. Use `info <name>` to confirm the browser sees the file.
