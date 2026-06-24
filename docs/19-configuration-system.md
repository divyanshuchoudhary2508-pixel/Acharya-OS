# AcharyaOS Phase 1, Feature 19 - Configuration System

## What this subsystem does

Feature 19 adds a tiny persistent configuration layer for kernel and shell
settings.

It can:

- list current settings
- get a value by key
- update a value by key
- save the settings to disk
- load the settings from disk

## Dependencies

This subsystem depends on:

- the filesystem, to persist settings
- the shell, to expose commands
- `kstring`, for text copying and comparison

## Architecture

The configuration store is a fixed array of key/value pairs kept in memory
and serialized as plain text in `key=value` form.

Default values are loaded first, then the config file is read on top of
them. That means the system always has usable defaults even if the file is
missing or incomplete.

## Folder structure

```text
kernel/include/config.h
kernel/config/config.c
docs/19-configuration-system.md
```

## Shell commands

- `config` shows all settings
- `get <key>` prints one setting
- `set <key> <value>` updates one setting
- `savecfg` writes settings to disk
- `loadcfg` reloads settings from disk

## Debugging guide

If settings do not persist:

1. Check that the filesystem is mounted.
2. Use `savecfg` after making changes.
3. Verify that `acharyaos.cfg` exists on the filesystem.

If a key appears to vanish, reload the config with `loadcfg` and then run
`config` again.
