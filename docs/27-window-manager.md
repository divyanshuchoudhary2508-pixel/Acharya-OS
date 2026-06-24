# AcharyaOS Phase 3, Feature 27 - Window Manager

## What this subsystem does

Feature 27 adds a small window manager on top of the framebuffer layer. It
keeps a registry of rectangular windows and renders them as bordered panels.

## Dependencies

This subsystem depends on:

- the framebuffer graphics core
- kernel string helpers
- shell command dispatch

## Architecture

The window manager is intentionally simple:

- a fixed-size window table
- lightweight metadata per window
- a renderer that paints windows in registration order
- a basic active-window marker

The shell exposes:

- `wminfo` to inspect the manager
- `wmdemo` to draw a sample layout

This is the first step toward later GUI work such as buttons, menus, and
desktop composition.

## Folder structure

```text
gui/include/window.h
gui/window.c
kernel/shell/shell.c
docs/27-window-manager.md
```

## Debugging guide

If `wminfo` reports the manager is not ready, check that `fb_init()` runs
before `wm_init()`.

If windows do not appear in the demo, check the framebuffer clear/fill path
and confirm the demo layout is being rendered after window creation.

If the active window marker is missing, confirm `wm_set_active()` was given a
valid id and that `wm_render()` is being called after state changes.
