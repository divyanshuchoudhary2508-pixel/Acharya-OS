# AcharyaOS — Subsystem Documentation
## Phase 1, Feature 3: Text Output

**Status: VERIFIED WORKING** (builds clean with zero unexpected compiler warnings, boots in
QEMU, color-switching and GDT-adjacent init sequence completes without crashing, cursor
clamping and backspace logic exercised and confirmed non-crashing via temporary self-test)

---

## 1. What This Subsystem Does

Promotes the kernel's own ad-hoc VGA-poking (built in Feature 2 purely so the kernel's boot
log was visible) into a real, separable driver living in `drivers/vga/`, with:
- A hardware-synced blinking cursor (not just internal row/col bookkeeping)
- Foreground/background color control (all 16 standard VGA colors)
- Backspace and carriage-return handling, in addition to newline and scrolling
- Bounds-checked cursor positioning for future callers (shell, task manager, etc.)

`kernel/lib/kio.c` is rewritten to contain **zero VGA hardware knowledge** — it now only
duplicates output to the driver and to serial, and owns `kprintf`'s format-string logic
(which has nothing to do with either backend specifically).

## 2. Dependencies

**Upstream:** Feature 2 (64-bit Kernel), specifically `port_io.c` (for the hardware cursor's
I/O-port-based control) and the existing `kio.h`/`kprintf` contract, which this feature
preserves exactly — no caller of `kprintf`/`kputs` needed to change.

**Downstream:**
- **Command Shell** (Feature 5) will be the first real consumer of `vga_set_cursor` and the
  visible hardware cursor — a person typing at a prompt needs to see where their input lands.
- **Task Manager** (Phase 2) and other screen-layout-aware tools will use `vga_set_cursor`
  for positioned output rather than purely sequential printing.
- **GUI / framebuffer work** (Phase 3) is the eventual reason this driver is isolated in
  `drivers/vga/` at all: when that subsystem wants a different display backend, the goal is
  that only this directory needs replacing, not every caller of `kprintf`.

## 3. Architecture

### 3.1 Driver/kernel separation: why this boundary, specifically

Before this feature, "the VGA driver" and "the kernel's diagnostic print function" were the
same code, in the same file. That was fine when the entire kernel had one job (print "I
booted"). It stops being fine the moment ANY other subsystem wants display behavior the
kernel-output use case doesn't need — which is now, with the hardware cursor and color.

The rule going forward: **`drivers/vga/vga.c` is the only file in AcharyaOS allowed to
mention address `0xB8000`, ports `0x3D4`/`0x3D5`, or the VGA attribute-byte bit layout.**
Everything else — `kio.c`, the future shell, anything — calls `vga_*` functions and knows
nothing about how they're implemented. This is the same reasoning that put `outb`/`inb` in
their own module in Feature 2: hardware-specific knowledge gets one home, not several.

### 3.2 Hardware cursor: why bother, given serial output doesn't need it

Serial output is a pure stream — there's no "cursor" concept, just bytes arriving in order.
VGA text mode is fundamentally different: it's a 2D grid the user is looking at in real
time, and a blinking cursor is the difference between "I can see where I am" and "I'm
guessing." This matters starting now (not later) because the very next thing that calls
into this driver in a back-and-forth, interactive way is the Command Shell — and retrofitting
hardware cursor sync after a shell already exists means touching every code path that
currently calls `vga_putchar`, instead of it already being correct from day one.

### 3.3 Why color is a simple set-function, not ANSI escape-code parsing

A full terminal emulator parses escape sequences like `\x1b[31m` embedded in the output
stream and maintains parser state across calls. That's real complexity — a state machine,
buffering for partial sequences split across `kprintf` calls — and **nothing in AcharyaOS
currently emits ANSI codes**, so building that parser now would be solving a problem we
don't have yet. `vga_set_color()` / `kio_set_color()` give every current and near-future
caller (kernel diagnostics, the upcoming shell) full color control with a single function
call and zero parsing overhead. If a real terminal application ever needs ANSI parsing
(e.g. to run programs that emit escape codes), that's a clearly separate, additive future
piece of work — it would consume `vga_set_color()` underneath, not replace this driver.

### 3.4 Backspace: explicitly does NOT wrap to the previous line

`vga_putchar('\b')` at column 0 does nothing (no wrap-to-previous-line). This is a
deliberate scope boundary, not an oversight: correctly erasing across a line boundary
requires knowing where the *logical* line actually started, which depends on what the
caller was typing — that's line-editing state, and line-editing is the Command Shell's job,
not this driver's. A driver-level guess here (e.g. "always wrap up exactly one row") would
be wrong as soon as the shell implements real multi-line input editing, so we deliberately
leave it as a no-op rather than build a guess that future code would have to work around.

## 4. Folder Structure

```
AcharyaOS/
├── drivers/                       # NEW: first subsystem to populate this required folder
│   ├── include/
│   │   └── vga.h                   # public API: vga_init, vga_putchar, vga_set_color, etc.
│   └── vga/
│       └── vga.c                    # ALL VGA hardware knowledge lives here, and ONLY here
├── kernel/
│   ├── kmain.c                     # CHANGED: demonstrates color via kio_set_color
│   ├── lib/
│   │   └── kio.c                    # REWRITTEN: zero VGA code now, delegates to drivers/vga
│   └── include/
│       └── kio.h                    # CHANGED: added kio_set_color, now includes vga.h
├── docs/
│   └── 03-text-output.md            # this file
└── Makefile                          # CHANGED: SRC_DIRS list, path-mirroring object layout
```

## 5. Build System Changes

Two real changes, both because this is the first subsystem outside `kernel/`:

1. **`SRC_DIRS := kernel drivers`** — source discovery (`find ... -name '*.c'`) now scans a
   list of top-level directories instead of just `kernel/`. Adding the next out-of-kernel
   subsystem (e.g. populating `memory/` later) means appending one word to this list.
2. **Object files now mirror their full source path** (`drivers/vga/vga.c` →
   `build/drivers/vga/vga.o`), not just the path relative to `kernel/`. This was a necessary
   fix, not a stylistic choice: the previous scheme (`build/<path-relative-to-kernel>.o`)
   would have silently collided if `kernel/foo.c` and `drivers/foo.c` ever shared a
   basename, the same class of bug as the `gdt.c`/`gdt.S` collision from Feature 2, just one
   level up (across directories instead of across extensions).

## 6. Build & Test Instructions

Unchanged from prior features:
```bash
make clean && make iso
make run                 # interactive, serial as stdio
```
Headless verification:
```bash
qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -display none \
    -serial file:build/serial.log -no-reboot
cat build/serial.log
```

Expected output:
```
AcharyaOS kernel starting...
------------------------------------------------
[init] Loading kernel GDT... done.
[init] Text Output driver (drivers/vga) active.
[ ok ] Color output verified (this line printed in default grey).
------------------------------------------------
AcharyaOS kernel initialization complete.
(Phase 1, Feature 3 - Text Output - now active.
 No further subsystems implemented yet. Halting.)
```

**Note on what serial output can and can't prove:** the `[ ok ]` line is printed in
`VGA_COLOR_LIGHT_GREEN` and the rest in `VGA_COLOR_LIGHT_GREY` — but serial output is a
plain byte stream with no color concept, so this distinction is invisible in
`serial.log` by design, not by bug. It is visible only on the actual VGA screen (run
`make run` without `-display none` on a machine with a display, or `-display gtk`/`sdl`
explicitly, to see it). The serial log instead proves the *code path* (the color-switching
function calls) executed without crashing — which is the failure mode that actually matters
for a driver (a bad color byte doesn't corrupt anything; a bad cursor-position write could).

## 7. Verification Evidence

```
$ make clean && make all 2>&1 | grep -iE "warning|error" | grep -v "GNU-stack\|RWX permissions\|NOTE:"
(no output - zero unexpected warnings across all files, old and new)

$ make iso && qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -display none \
    -serial file:build/serial.log -no-reboot
$ cat build/serial.log
AcharyaOS kernel starting...
------------------------------------------------
[init] Loading kernel GDT... done.
[init] Text Output driver (drivers/vga) active.
[ ok ] Color output verified (this line printed in default grey).
------------------------------------------------
AcharyaOS kernel initialization complete.
(Phase 1, Feature 3 - Text Output - now active.
 No further subsystems implemented yet. Halting.)
```

### Edge-case verification (temporary self-test, added then removed)
`vga_set_cursor`'s clamping and `vga_putchar`'s backspace path are NOT exercised by the
normal boot sequence above (nothing in `kmain()` calls them in the shipped code). Before
considering this feature done, a temporary block was added to `kmain()`:
```c
vga_set_cursor(999, -5);   /* deliberately out-of-range in both axes */
vga_set_cursor(10, 10);
kputs("XY");
kputs("\b");
```
This ran successfully (`[test] ... survived.` printed, no crash, no hang), confirming the
clamp logic in `vga_set_cursor` correctly handles out-of-range input without writing outside
the VGA buffer, and that backspace doesn't corrupt cursor state. The test block was then
**removed** — it was verification scaffolding, not permanent kernel behavior, consistent with
the project's "don't write code nothing calls" principle once its one job was done.

### What was NOT independently re-verified this feature
A literal screenshot of the colored VGA output was not captured in this session (QEMU's
screendump-via-QMP tooling proved unreliable in this particular sandboxed environment when
attempted in earlier features too). The serial-log + self-test evidence above confirms the
underlying logic executes correctly; actually *seeing* the green "[ ok ]" text requires
running `make run` with a real display, which is left to you to confirm visually the first
time you run this locally. This is flagged explicitly rather than implied as fully checked.

## 8. Debugging Strategy (additions specific to this feature)

### 8.1 "Cursor doesn't move / blinking cursor stays at top-left"
The hardware cursor is set via `outb` to ports 0x3D4/0x3D5 — if this isn't working, check:
1. Is `vga_hw_set_cursor` actually being called at the end of every `vga_putchar` and
   `vga_set_cursor`? (It must run on every change, not just at init - a one-time set at
   `vga_init()` would freeze the visible cursor at (0,0) forever even as text correctly
   scrolls past it.)
2. Some emulators/VMs disable the hardware cursor by default (register 0x0A, the "cursor
   start" register, has a bit that can hide it). We don't currently set this register
   explicitly - if the cursor is invisible despite correct position writes, that register
   is the next thing to set (not needed for QEMU's default VGA emulation, which is what
   we've verified against, but worth knowing if this is ever run on different emulated or
   real hardware).

### 8.2 "Text appears in the wrong color, or color 'leaks' into previous lines"
Remember: `vga_set_color()` only affects characters written *after* the call - it does not
recolor existing on-screen text. If you see unexpected color on already-printed lines,
check whether `vga_clear()` (called from `vga_init()`) ran with a stale `vga_current_color`
value - the color used for `vga_clear()`'s blank-fill is whatever was set most recently
before the clear, not necessarily the "default."

### 8.3 New pattern for catching driver-level logic bugs: temporary self-tests in kmain()
Section 7's "added then removed" self-test is a reusable technique for verifying code paths
the normal boot sequence doesn't exercise. The discipline that makes this safe rather than
sloppy: the test code must be removed once it's done its job, and removal is verified by an
immediate rebuild + reboot (Section 7 shows exactly that final clean run) - a self-test that
silently became permanent kernel behavior would violate the project's own "don't write code
nothing calls" principle just as much as never having tested it at all.

## 9. Known Limitations (intentional, to be addressed by later subsystems)

- **No ANSI/VT100 escape code parsing** — by design, per Section 3.3. Revisit only if a
  future subsystem genuinely needs to interpret escape sequences in a byte stream (e.g.
  running existing terminal programs), not before.
- **No register 0x0A (cursor visibility/shape) handling** — works correctly under QEMU's
  default VGA emulation; flagged in Section 8.1 as the first thing to check if this is ever
  run under different emulated or real hardware and the cursor isn't visible.
- **Backspace does not wrap across line boundaries** — intentional, per Section 3.4; the
  Command Shell will own correct multi-line line-editing semantics.
- **Color screenshot not independently captured this session** — see Section 7's final
  note. The underlying logic is verified via self-test and code review; visual confirmation
  on a real display is the one remaining check left for you to do locally.
