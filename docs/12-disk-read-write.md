# Phase 1, Feature 12: Disk Read/Write

## What It Does

This feature adds raw block access to ATA disks using legacy PIO ports. It is
the storage layer that the future filesystem will sit on top of.

The driver can:

- probe the primary ATA master
- read sectors
- write sectors
- report basic device information
- run a read/write self-test on a safe scratch sector

## Dependencies

- Port I/O: ATA uses the legacy primary IDE port range.
- Kernel memory/string helpers: used for buffer handling and diagnostics.
- Shell: exposes `disk` and `disktest`.

## Architecture

Files:

```text
drivers/include/ata.h
drivers/disk/ata.c
docs/12-disk-read-write.md
```

Hardware path:

```text
ATA disk -> PIO registers 0x1F0-0x1F7 -> polling -> read/write buffer
```

This first version uses polling rather than interrupts. That is intentional:
polling is easier to debug and perfectly adequate for a single-drive hobby OS
at this stage.

The boot-time probe does not panic if no ATA device exists. That keeps ISO-only
QEMU boots safe. To verify actual writes, attach a writable disk image in QEMU
and use the `disktest` shell command.

## Build

```sh
make clean
make
make iso
```

## QEMU Test Plan

With an attached writable disk image:

```sh
make run
```

Then type:

```text
disk
disktest
```

Expected behavior:

- Kernel prints `[init] Probing ATA disk... done.`
- `disk` reports either a detected ATA drive or no drive present.
- `disktest` writes a temporary pattern to sector 2048, reads it back, and
  restores the original contents.

## Debugging Strategy

- If the probe says no disk, confirm the QEMU command line includes a writable
  IDE disk image, not just the boot ISO.
- If reads fail but a disk is present, check the status polling loop and the
  LBA28 setup sequence.
- If writes fail, verify the disk image is writable and not a CD-ROM.
- If the self-test passes but later filesystem code fails, the bug is likely in
  the filesystem layer rather than ATA PIO.
