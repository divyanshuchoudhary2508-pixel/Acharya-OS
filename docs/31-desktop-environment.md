# AcharyaOS Phase 3, Feature 31 - Desktop Environment

## What this subsystem does

Feature 31 adds a minimal desktop environment layer on top of the existing
framebuffer, window manager, menu, and button subsystems. It renders a simple
background, default icons, and a taskbar so AcharyaOS can present a coherent
graphical surface rather than isolated GUI primitives.

## Dependencies

This subsystem depends on:

- the framebuffer graphics core
- the window manager
- the menu layer
- the button layer
- kernel string helpers
- shell command dispatch

## Architecture

The desktop is intentionally small and AcharyaOS-native:

- fixed-size icon registry
- simple taskbar rendering
- built-in default icons
- shell inspection and demo commands

The shell exposes:

- `dskinfo` to inspect desktop state
- `dskdemo` to redraw the desktop surface

## Folder structure

```text
gui/include/desktop.h
gui/desktop.c
kernel/shell/shell.c
kernel/kmain.c
docs/31-desktop-environment.md
```

## Debugging guide

If `dskinfo` says the desktop is not ready, confirm `fb_init()`, `wm_init()`,
`menu_init()`, and `btn_init()` all ran before `desktop_init()`.

If the surface renders blank, check the framebuffer clear and the window/menu
render paths first.

If icons do not appear, confirm `desktop_add_default_icons()` ran and the icon
registry still has capacity.
