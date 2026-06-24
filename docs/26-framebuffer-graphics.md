# AcharyaOS Phase 3, Feature 26 - Framebuffer Graphics

## What this subsystem does

Feature 26 adds a framebuffer graphics core. It provides a pixel-oriented
surface for future GUI work while keeping text mode intact.

## Dependencies

This subsystem depends on:

- kernel text output for diagnostics
- bounded string helpers
- the shell command dispatcher

It does not yet depend on a real bootloader framebuffer handoff. That comes
later when the boot path is extended to pass display mode information through.

## Architecture

The graphics layer owns a small software framebuffer with a fixed default
resolution. The point of this stage is to establish the API boundary and the
rendering model:

- initialize a framebuffer surface
- clear it to a solid color
- draw individual pixels
- fill rectangles
- generate a simple demo pattern

The shell exposes:

- `gfxinfo` to inspect the surface
- `gfxdemo` to paint the demo pattern

## Folder structure

```text
gui/include/framebuffer.h
gui/framebuffer.c
kernel/shell/shell.c
docs/26-framebuffer-graphics.md
```

## Debugging guide

If `gfxinfo` says the framebuffer is not initialized, confirm `fb_init()` runs
from kernel startup.

If the demo does not render later on real framebuffer hardware, check the
loader integration and confirm the buffer address is mapped and writable.

If drawing functions return `-1`, check bounds and the current framebuffer
dimensions.
