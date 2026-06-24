# AcharyaOS Phase 3, Feature 30 - Menus

## What this subsystem does

Feature 30 adds a lightweight menu layer on top of the framebuffer and window
manager. It provides a compact registry for top-level menus and a simple demo
renderer so later GUI work can build menu bars and dropdowns without changing
the core data model.

## Dependencies

This subsystem depends on:

- the framebuffer graphics core
- the window manager
- kernel string helpers
- shell command dispatch

## Architecture

Menus are intentionally data-driven:

- fixed-size menu registry
- simple menu metadata
- menu item storage with action identifiers
- demo renderer for the shell-facing graphics path

The shell exposes:

- `meninfo` to inspect menu state
- `mendemo` to draw the menu demo layout

## Folder structure

```text
gui/include/menu.h
gui/menu.c
kernel/shell/shell.c
docs/30-menus.md
```

## Debugging guide

If `meninfo` says menus are not ready, confirm `fb_init()` and `wm_init()`
run before `menu_init()`.

If the demo does not render, check that the framebuffer and window manager
draw paths are still functioning and that the demo is called after startup.

If menu items do not appear, confirm the menu registry still has capacity and
that `menu_add_item()` is being called with a live menu id.
