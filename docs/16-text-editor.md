# AcharyaOS Phase 1, Feature 16 - Text Editor

## What this subsystem does

Feature 16 adds a small interactive text editor that runs inside the kernel
shell. It can:

- load a file from the filesystem
- show and edit text line by line
- move a cursor through the buffer
- save changes back to disk

This is a practical step toward real user tools without jumping ahead to a
full graphical application.

## Dependencies

This subsystem depends on:

- the shell, for launching the editor
- keyboard input, for editor commands
- `fs`, for load/save operations
- `kio` and `vga`, for output and color
- `kstring`, for buffer manipulation

## Architecture

The editor keeps a small in-memory array of lines. That keeps the design
easy to understand and avoids needing a full screen buffer or complex text
layout engine at this stage.

Supported editor actions:

- `show`
- `i <text>`
- `a <text>`
- `d`
- `n`
- `p`
- `s`
- `wq`
- `q`

## Folder structure

```text
kernel/include/editor.h
kernel/editor/editor.c
docs/16-text-editor.md
```

## Build notes

The Makefile already discovers new `.c` files automatically, so the editor
builds as soon as it is placed under `kernel/`.

## Debugging guide

If the editor does not start:

1. Confirm the shell command is `edit <filename>`.
2. Check that the filesystem is mounted.
3. Verify keyboard input still reaches the shell.

If saving fails:

1. Make sure the file name is not empty.
2. Check the filesystem status with `fs`.
3. Try a smaller file to stay under the editor buffer limit.
