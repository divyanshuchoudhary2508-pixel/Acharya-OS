# Feature 34: Settings Framework

## What it does
The settings subsystem provides a lightweight, panel-based GUI window for browsing kernel configuration categories.
It is designed as a reusable shell-and-desktop target rather than a one-off screen.

## Architecture
The implementation is split into four layers:

- `settings.c`: window entry point, render orchestration, stats
- `settings_data.c`: config-backed panel data
- `settings_ui.c`: small drawing helpers for the frame, sidebar, and content area
- `settings_panels.c`: panel assembly wrapper

## Current panels
- Display
- Audio
- Network
- Users
- About

## Shell integration
Use the following commands:

- `settings`: render the settings window
- `settinfo`: print settings status
- `settdemo`: render the settings demo view

## Dependencies
The subsystem depends on:

- framebuffer graphics
- kernel config storage
- the desktop/icon launch path

## Notes
This is intentionally minimal. It establishes the structure first, then can grow sliders,
editable fields, and save-to-disk behavior in later passes.

