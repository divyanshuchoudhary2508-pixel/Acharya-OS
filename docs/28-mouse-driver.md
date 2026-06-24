# AcharyaOS Phase 3, Feature 28 - Mouse Driver

## What this subsystem does

Feature 28 adds PS/2 mouse support. The driver enables the auxiliary device,
collects 3-byte movement packets, and keeps a small state snapshot for the
kernel to inspect.

## Dependencies

This subsystem depends on:

- the IDT
- the PIC
- port I/O helpers
- kernel text output for debugging

## Architecture

The driver is intentionally minimal:

- the PS/2 controller is programmed for the auxiliary mouse device
- IRQ12 is unmasked after the handler is registered
- packets are accumulated one byte at a time
- the shell can inspect the latest movement and button state

The shell exposes:

- `mouse` to show driver state

## Folder structure

```text
drivers/include/mouse.h
drivers/mouse/mouse.c
kernel/shell/shell.c
docs/28-mouse-driver.md
```

## Debugging guide

If the driver never becomes ready, check that the controller accepts the
auxiliary-device enable sequence and that IRQ12 is unmasked.

If packets never complete, confirm the first-byte sync bit is present and that
the PS/2 mouse is enabled before trying to read packets.

If the shell shows no movement, verify the interrupt path is firing and that
the packet bytes are being consumed in order.
