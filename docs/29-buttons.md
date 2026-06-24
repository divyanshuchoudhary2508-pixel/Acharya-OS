# AcharyaOS Phase 3, Feature 29 - Buttons

## What this subsystem does

Feature 29 adds a lightweight button control layer. Buttons are rectangular
GUI controls that can be rendered on top of windows and later connected to
mouse input and application actions.

## Dependencies

This subsystem depends on:

- the framebuffer graphics core
- the window manager
- kernel string helpers
- shell command dispatch

## Architecture

Buttons are intentionally small and data-driven:

- fixed-size button registry
- window association for future input routing
- geometry and color state
- a pressed/unpressed visual state

The shell exposes:

- `btninfo` to inspect button state
- `btndemo` to draw a sample button layout

This feature does not yet do font rendering or click routing. It establishes
the control primitive first.

## Folder structure

```text
gui/include/button.h
gui/button.c
kernel/shell/shell.c
docs/29-buttons.md
```

## Debugging guide

If `btninfo` shows zero buttons, confirm `btn_init()` ran during startup.

If the demo renders windows but not buttons, check the framebuffer fill path
and confirm button rectangles are inside the demo windows.

If a pressed state does not change, verify `btn_set_pressed()` is being called
with a valid button id.
