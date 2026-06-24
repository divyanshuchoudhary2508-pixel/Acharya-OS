# AcharyaOS Phase 1, Feature 20 - Package Installer

## What this subsystem does

Feature 20 adds a tiny package installer that reads a text manifest from
the filesystem, extracts its files, and records that the package was
installed.

## Dependencies

This subsystem depends on:

- the filesystem, to read package manifests and write extracted files
- the shell, to expose installer commands
- `kstring`, for parsing and string handling

## Architecture

The package format is intentionally small and educational:

- `PKG1` header
- `package=<name>`
- `version=<version>`
- one or more `file=<filename>|<content>` lines
- `end`

Because AcharyaOS still has a flat filesystem, installed files are written
directly by filename instead of into package-specific directories.

## Folder structure

```text
kernel/include/package.h
kernel/package/package.c
docs/20-package-installer.md
```

## Shell commands

- `pkg` shows installed package records
- `install <manifest-file>` installs a package from a manifest file
- `remove <package-name>` removes the package record

## Debugging guide

If installation fails:

1. Check that the manifest file exists.
2. Verify the manifest begins with `PKG1`.
3. Make sure each file line has the `file=name|content` form.
4. Confirm the filesystem has space for the extracted files.

If a package shows up but its files are missing, inspect the manifest to
make sure the filenames were valid and unique.
