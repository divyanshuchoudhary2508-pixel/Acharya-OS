# Phase 1, Feature 13: Simple Filesystem

## What It Does

This feature adds a tiny filesystem on top of raw ATA sectors. It supports:

- mounting a filesystem
- listing files
- reading file contents
- writing file contents
- deleting files
- basic integrity checking

## Dependencies

- Disk Read/Write: provides raw sector access.
- Shell and Text Output: provide diagnostics and file commands.

## Architecture

Files:

```text
fs/include/fs.h
fs/fs.c
docs/13-simple-filesystem.md
```

On-disk layout:

```text
LBA 4096 : superblock
LBA 4097 : file table
LBA 4098+ : file data area
```

Files are stored as contiguous extents. That is not the most space-efficient
design, but it is compact and easy to debug at this stage.

## Build

```sh
make clean
make
make iso
```

## QEMU Test Plan

Attach a writable disk image, then:

```sh
make run
```

Try:

```text
fs
ls
writefs note hello world
ls
cat note
rmfs note
ls
```

Expected behavior:

- Kernel prints `[init] Mounting filesystem... done.`
- `fs` shows whether the filesystem is mounted.
- `writefs` creates or overwrites a file.
- `cat` prints file contents.
- `rmfs` deletes the file.

## Debugging Strategy

- If the filesystem is not mounted, confirm the ATA layer detects a writable
  disk image.
- If formatting happens unexpectedly, the on-disk magic or version does not
  match the current format.
- If `writefs` fails, check the file-size cap and whether the data region has
  enough contiguous space.
- If `cat` prints garbage, confirm the file length and read length match.
