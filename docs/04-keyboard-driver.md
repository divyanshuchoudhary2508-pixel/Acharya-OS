# Phase 1, Feature 4: Keyboard Driver

## What It Does

The keyboard driver turns PS/2 keyboard interrupts into characters the kernel
can consume. In QEMU this uses the classic PC path: keyboard controller IRQ1,
legacy 8259 PIC, IDT vector 33, then a C interrupt handler.

## Dependencies

- Feature 1 bootloader: reaches 64-bit mode and calls `kmain`.
- Feature 2 kernel core: provides `kmain`, GDT, panic handling, and Make build.
- Feature 3 text output: lets us print initialization status and echo typed
  characters for verification.

## Architecture

This feature introduces three pieces of shared interrupt infrastructure:

- `kernel/arch/x86_64/idt.c`: owns the Interrupt Descriptor Table.
- `drivers/pic/pic.c`: remaps the legacy PIC so IRQ0-15 become vectors 32-47.
- `drivers/keyboard/keyboard.c`: registers the IRQ1 handler and reads port
  `0x60`.

The keyboard interrupt handler is intentionally small. It reads one scancode,
tracks Shift state, translates known set-1 scancodes to ASCII, pushes the
character into a fixed ring buffer, sends PIC EOI, and returns with `iretq`.
Everything slower or more semantic belongs to future shell/editor code.

## Files

```text
kernel/include/idt.h
kernel/arch/x86_64/idt.c
kernel/arch/x86_64/idt_load.S
kernel/arch/x86_64/isr_stubs.S
drivers/include/pic.h
drivers/pic/pic.c
drivers/include/keyboard.h
drivers/keyboard/keyboard.c
docs/04-keyboard-driver.md
```

## Build

```sh
make clean
make
make iso
```

The existing Makefile discovers new `.c` and `.S` files automatically under
`kernel/` and `drivers/`.

## QEMU Test

```sh
make run
```

Expected behavior:

1. The kernel prints Feature 4 initialization status.
2. It enables interrupts with `sti`.
3. Typed printable keys echo to the VGA screen.
4. Shift changes letters/symbols.
5. Backspace erases one character.

## Debugging Strategy

- If QEMU resets immediately, suspect a bad IDT gate, wrong code selector, or
  broken interrupt stack frame.
- If the first key works but later keys do not, suspect a missing PIC EOI.
- If no keys work, confirm `pic_init()`, `idt_init()`, `keyboard_init()`, and
  `sti` run in that order.
- If letters are wrong, inspect the scancode translation tables.
- Use `make run` serial output to see panic messages even when VGA is garbled.
