# Phase 1, Feature 5: Command Shell

## What It Does

The command shell is the first interactive user-facing loop in AcharyaOS. It
reads characters from the keyboard driver, performs simple line editing, parses
one command line at a time, and runs built-in kernel commands.

This is still a kernel-resident shell. AcharyaOS does not yet have memory
allocation, processes, system calls, or a filesystem, so a true user-space
shell would be premature.

## Dependencies

- Text Output: prints prompts, command output, and backspace updates.
- Keyboard Driver: supplies translated ASCII characters through
  `keyboard_getchar()`.
- Interrupts/PIC/IDT: allow keyboard input to arrive asynchronously.
- `kstring`: provides small freestanding string helpers.

## Architecture

The shell is intentionally layered above the keyboard driver:

- `drivers/keyboard/keyboard.c` knows scancodes, IRQ1, and port `0x60`.
- `kernel/shell/shell.c` knows prompts, editable lines, and commands.

The current command set is deliberately small:

- `help`
- `clear`
- `echo <text>`
- `version`
- `halt`

## Files

```text
kernel/include/shell.h
kernel/shell/shell.c
docs/05-command-shell.md
```

Small supporting change:

```text
kernel/include/kstring.h
kernel/lib/kstring.c
```

adds `strcmp()` for command dispatch.

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
help
version
echo hello acharya
clear
help
halt
```

Expected behavior:

- A colored `acharyaos>` prompt appears.
- Backspace edits the current line.
- Enter submits the command.
- Unknown commands print a readable error.
- `halt` disables interrupts and halts the CPU.

## Debugging Strategy

- If no prompt appears, check that `shell_run()` is called after `sti`.
- If keys do not appear, debug Feature 4 first: IDT, PIC, keyboard IRQ, EOI.
- If Enter does nothing, check that the keyboard table maps scancode `0x1C`
  to `'\n'`.
- If backspace moves oddly, keep the bug in shell line editing, not the VGA
  driver. The shell owns command-line editing semantics.
