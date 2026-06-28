# Feature 35: Graphical File Explorer

## What it does
The file explorer is a graphical view of the simple filesystem. It renders file tiles
and a status summary so the desktop has a launchable file-management surface.

## Dependencies
- framebuffer graphics
- filesystem listing APIs
- shell command dispatch
- desktop icon launcher

## Current behavior
- `files` renders the explorer view
- `fileinfo` prints status
- `filedemo` redraws the current view

## Design note
This version is intentionally lightweight because AcharyaOS currently has a flat
filesystem and no font renderer. It gives us the correct application shape first.

