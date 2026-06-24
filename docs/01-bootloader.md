# AcharyaOS — Subsystem Documentation
## Phase 1, Feature 1: Bootloader

**Status: VERIFIED WORKING** (boots in QEMU, reaches 64-bit long mode, hands off to kernel stub successfully — see "Verification Evidence" below)

---

## 1. What This Subsystem Does

The bootloader is responsible for the journey from "CPU just powered on, knows nothing" to
"64-bit kernel code is running, with paging enabled, on a known-good stack." Concretely, for
AcharyaOS, this subsystem:

1. Declares itself as Multiboot2-compliant so GRUB can find and load it.
2. Receives control from GRUB in 32-bit protected mode.
3. Builds the page tables required for 64-bit paging (mandatory for long mode).
4. Performs the exact, ordered sequence of privileged operations to enter 64-bit long mode.
5. Hands off to `kmain()`, our C kernel entry point, passing it the Multiboot2 info pointer.

This subsystem's job ends the moment `kmain()` starts running. Everything after that —
text output, memory management, scheduling — belongs to later subsystems.

## 2. Dependencies

**Upstream dependencies:** None. This is the root of AcharyaOS's dependency graph — the
first AcharyaOS-owned code the CPU ever executes.

**External dependencies (not part of AcharyaOS):**
- GRUB (via `grub-mkrescue`) — handles real-mode BIOS interaction, disk I/O to load our
  binary, the A20 line, and getting the CPU into 32-bit protected mode before handing off
  to us. GRUB is a bootloader; AcharyaOS's *own* bootloader logic begins where GRUB's ends —
  at the 32-bit protected mode entry point in `boot.S`.

**Downstream dependents:** Every other subsystem. `kmain()` (currently a stub) is the only
thing this subsystem hands control to. Once the real kernel subsystem (Phase 1, Feature 2)
exists, this file's `call kmain` line doesn't change — it now calls a real kernel instead
of a stub.

## 3. Architecture

### 3.1 Why GRUB + Multiboot2 instead of a hand-written MBR boot sector

A from-scratch OS could in principle write its own 512-byte MBR boot sector and a
second-stage loader. We deliberately chose not to, for this project, and it's worth being
explicit about the tradeoff:

| Aspect | Raw MBR (512-byte boot sector) | GRUB + Multiboot2 |
|---|---|---|
| Who handles A20, disk I/O, BIOS calls | You, by hand, in 16-bit real mode | GRUB |
| Time to first kernel C code running | Long — needs a hand-rolled 2nd-stage loader | Fast |
| Educational value of the *boot.S* code we DO write | N/A (different code entirely) | High — every byte of the 32-bit-to-64-bit transition is ours |
| Risk of subtle real-hardware quirks (disk geometry, etc) | High | Low — GRUB is battle-tested across real hardware |

We are not skipping anything that teaches us about long-mode transition, paging, GDTs, or
calling conventions — `boot.S` does all of that ourselves. We're skipping the *legacy BIOS
disk and real-mode trivia*, which is a different (and much less generally useful) body of
knowledge. This can be revisited as an optional exercise later.

### 3.2 The 32-bit to 64-bit transition (the actual hard part)

x86_64 has no single "go 64-bit" instruction. The mandatory sequence, all done by hand in
`boot.S`, is:

```
1. Build page tables (paging is REQUIRED for long mode, no way around it)
2. Set CR4.PAE          (enable Physical Address Extension)
3. Load CR3              (point the CPU at our page tables)
4. Set EFER.LME via MSR  (arm Long Mode Enable)
5. Set CR0.PG             (turn on paging -- enters "IA-32e compatibility mode")
6. Load a 64-bit GDT      (need a code segment descriptor with the L-bit set)
7. Far jump into that segment  (THIS instruction is the actual mode switch)
```

Misordering or skipping any step causes a triple fault (silent CPU reset) with no error
message — which is why we test in QEMU with serial logging at every stage rather than
guessing.

### 3.3 Memory map decision: identity-mapped first 1 GiB

For this bootloader stage only, we map virtual address == physical address for the first
1 GiB, using six 2 MiB "huge pages" worth of Page Directory entries (512 entries × 2 MiB =
1 GiB). This is the simplest mapping that satisfies long mode's hard requirement of having
*some* valid paging structure, and it's enough room for our kernel binary (loaded at 1 MiB)
plus headroom.

**This is intentionally temporary.** A real kernel uses a "higher-half" design — kernel code
lives at a high virtual address (commonly `0xFFFFFFFF80000000`+) while physical memory below
1 MiB stays reserved for legacy BIOS structures, and userspace gets the low address range.
The Virtual Memory subsystem (Phase 1, Feature 7) will replace this identity map with that
proper layout. We're flagging this now so it's not a surprise later — and so it's clear the
two linker warnings we got (RWX segment, executable stack) are a *consequence* of this
deliberately simple initial mapping, not a separate bug.

### 3.4 Kernel load address: 1 MiB

`linker.ld` places the kernel at physical/virtual address `0x100000` (1 MiB). This is the
traditional safe load point for Multiboot kernels — everything below 1 MiB is a minefield of
BIOS data areas, the VGA text buffer (`0xB8000`), and option ROMs.

## 4. Folder Structure

```
AcharyaOS/
├── boot/
│   └── arch/x86_64/
│       ├── boot.S        # Multiboot2 header + 32-to-64-bit transition code
│       └── linker.ld      # Memory layout / linker script
├── kernel/
│   ├── kmain.c             # TEMPORARY stub kernel entry (proves handoff worked)
│   └── include/            # (empty for now; reserved for kernel.h etc.)
├── tools/
│   └── grub.cfg             # GRUB menu config embedded into the ISO
├── docs/
│   └── 01-bootloader.md     # This file
└── Makefile                  # Build + run targets
```

## 5. Build Instructions

### Required tools (Debian/Ubuntu package names)
```bash
sudo apt-get install -y gcc binutils make nasm \
    grub-pc-bin grub-common xorriso mtools \
    qemu-system-x86
```
(`nasm` isn't actually invoked yet since `boot.S` is assembled via `gcc`/GNU `as`, but it's
kept available for future modules that may want raw NASM syntax.)

### Build the raw kernel binary
```bash
make all
```
This produces `build/acharyaos.bin` — an ELF64 executable, but NOT yet bootable on its own
(GRUB needs the ISO wrapper below to actually load it via BIOS).

### Build the bootable ISO
```bash
make iso
```
Produces `build/acharyaos.iso`, an ISO 9660 image with a hybrid MBR boot sector that real
hardware or QEMU can boot from directly.

### Clean
```bash
make clean
```

## 6. QEMU Testing Instructions

### Interactive (see the VGA screen + get a serial console in your terminal)
```bash
make run
```
This runs:
```bash
qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -display none \
    -serial mon:stdio -no-reboot -no-shutdown
```
You should see (over the serial-as-stdio connection):
```
[AcharyaOS] Boot stub reached. 64-bit long mode verified OK.
[AcharyaOS] Multiboot2 handoff successful. Halting.
```

### Headless / scripted verification (what we used to verify this build)
```bash
qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -display none \
    -serial file:build/serial.log -no-reboot
cat build/serial.log
```

### If you have a real graphical environment
Drop `-display none` and add nothing else — QEMU will open a window showing the VGA
text-mode output: `AcharyaOS boot stub: 64-bit long mode reached. Bootloader verified OK.`

## 7. Verification Evidence

Build performed and tested in this session:
```
$ make clean && make iso
... (builds boot.o, kmain.o, links acharyaos.bin, wraps in acharyaos.iso) ...
Built bootable ISO: build/acharyaos.iso

$ file build/acharyaos.bin
build/acharyaos.bin: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), statically linked, with debug_info, not stripped

$ file build/acharyaos.iso
build/acharyaos.iso: ISO 9660 CD-ROM filesystem data (DOS/MBR boot sector) 'ISOIMAGE' (bootable)

$ qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -display none -serial file:build/serial.log -no-reboot
$ cat build/serial.log
[AcharyaOS] Boot stub reached. 64-bit long mode verified OK.
[AcharyaOS] Multiboot2 handoff successful. Halting.
```

This confirms, in order: GRUB found and parsed our Multiboot2 header, loaded the kernel at
1 MiB, jumped to `_start`; our hand-written page tables were built correctly (no triple
fault); PAE, LME, and PG were set in the right order; the far jump into 64-bit code
succeeded; the stack was valid; the call into `kmain()` (a real cross-language, cross-mode
function call) worked; and `kmain()` executed real C code (port I/O via inline asm) correctly.

## 8. Debugging Strategy

Bootloader bugs are uniquely painful: there's no OS yet to catch a crash, no stack trace, no
`printf` until you build one. Here's the toolkit, roughly in the order you should reach for
each:

### 8.1 "Nothing happens / instant reboot" (most common symptom: a triple fault)
QEMU by default silently resets on a triple fault, which looks identical to "nothing
happened." Fixes:
- Always run with `-no-reboot -no-shutdown` during development (already in the `run`
  target) so QEMU halts instead of silently rebooting, making the failure visible.
- Add `-d int,cpu_reset` to the QEMU command line to log every interrupt and reset event to
  stderr. A triple fault shows up here even with no other output.

### 8.2 Bisecting *where* in boot.S things went wrong
Insert `outb`-based serial prints (or even simpler: write a single recognizable byte pattern
to memory you can inspect) immediately after each major step (page tables built, PAE set,
CR3 loaded, LME set, paging enabled, GDT loaded, post-far-jump). Since we don't have
`kmain()`'s `serial_print` available until *after* the transition, the earliest checkpoints
need raw inline `outb` to port `0x3F8` directly in assembly:
```asm
mov $0x3F8, %dx
mov $'A', %al    /* checkpoint marker - change the letter per checkpoint */
out %al, %dx
```
Run with `-serial file:build/serial.log` and `cat` the log after each test — if you see
`A` but not `B`, the bug is between those two checkpoints.

### 8.3 Inspecting CPU state directly with QEMU's built-in monitor
```bash
qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -s -S
```
- `-S` pauses the CPU at startup (doesn't auto-run)
- `-s` opens a GDB stub on `localhost:1234`

Then in another terminal:
```bash
gdb
(gdb) target remote localhost:1234
(gdb) info registers       # check CR0, CR3, CR4 state by hand if needed via 'info all-registers'
(gdb) stepi                # single-step one instruction at a time
```
This is the most powerful tool for genuinely mysterious failures — you can single-step
through the entire mode transition and watch registers change in real time.

### 8.4 Common specific mistakes (in rough order of likelihood)
1. **Forgetting `-mno-red-zone`** when later code adds interrupt handlers — causes stack
   corruption that's very hard to trace back to its real cause.
2. **Page table entries missing the Present or Writable bit** — silent triple fault the
   instant paging is enabled, with no indication of which entry was wrong.
3. **GDT descriptor encoding mistakes** — the 64-bit GDT format packs flags into specific
   bit positions; a single wrong bit (especially the L-bit, bit 53) causes a triple fault
   right at the far jump.
4. **Stack not 16-byte aligned** when calling into C — x86_64 SysV ABI requires it; C code
   that uses SSE instructions (even via the compiler's own optimizations) can crash
   mysteriously if this is violated. (Our `-O0` build sidesteps most of this for now, but
   it will matter again once optimizations are turned on.)

## 9. Known Limitations (intentional, to be addressed by later subsystems)

- **RWX memory everywhere**: every page is marked Present+Writable+Executable. The Virtual
  Memory subsystem will introduce proper `.text` (R+X), `.rodata` (R), `.data`/`.bss` (R+W)
  separation.
- **Only 1 GiB identity-mapped**: fine for now; will be replaced by a higher-half kernel
  mapping plus a real physical memory map parsed from the Multiboot2 info structure.
- **No interrupt handling yet**: a real fault right now (e.g. a bad memory access) is a
  silent triple fault with no diagnostic output. The IDT (Interrupt Descriptor Table) setup
  belongs to a later subsystem and will give us actual fault messages.
- **kmain.c is a stub**, not the real kernel — it exists solely to prove this subsystem
  works, and will be replaced wholesale when Phase 1, Feature 2 (64-bit Kernel) begins.
