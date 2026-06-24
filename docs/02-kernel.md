# AcharyaOS — Subsystem Documentation
## Phase 1, Feature 2: 64-bit Kernel

**Status: VERIFIED WORKING** (builds clean with zero compiler warnings, boots in QEMU,
GDT swap succeeds without crashing, kprintf output confirms correct staged initialization)

---

## 1. What This Subsystem Does

This feature turns the bootloader's one-shot verification stub into the actual skeleton of
a kernel: a staged initialization sequence, a permanent kernel-owned GDT, a freestanding
string/memory library, and a real output API (`kprintf`) that every future subsystem will
use to report its own boot progress and errors.

Concretely, it replaces `kmain.c`'s old "print one message and halt" body with an
orchestrator that calls named `xxx_init()` functions in a documented order, and gives the
kernel its first piece of genuinely reusable internal infrastructure.

## 2. Dependencies

**Upstream:** Phase 1, Feature 1 (Bootloader). This subsystem assumes `boot.S` has already
gotten the CPU into 64-bit long mode with a working stack and has called `kmain()` per the
System V calling convention. Nothing here re-does or second-guesses that work.

**Downstream:** Every later subsystem. Specifically:
- The upcoming **Timer Interrupts** subsystem will need an IDT, and will follow the exact
  same pattern established here (a `*_init()` function called from `kmain()`, with its own
  C struct definitions for the hardware-mandated layout).
- **Memory Allocator / Virtual Memory** will be the first real consumer of the Multiboot2
  info pointer that `kmain()` currently accepts but ignores.
- **User-Space Programs** will be the first consumer of the user-mode GDT segments
  (`GDT_SEGMENT_USER_CODE` / `GDT_SEGMENT_USER_DATA`) defined now but unused until then.
- Essentially every subsystem from here on will call `kprintf()` for diagnostics and
  `kernel_panic()` for unrecoverable errors.

## 3. Architecture

### 3.1 kmain() as an orchestrator, not a place where logic lives

```c
void kmain(uint64_t multiboot_info_ptr) {
    kio_init();        // Step 1: output, so everything after this can report itself
    gdt_init();         // Step 2: permanent GDT, replacing boot.S's transitional one
    // (future steps slot in here, each with its own documented reason for its position)
}
```
This mirrors the structure of real kernels (Linux's `start_kernel()`, the BSDs' `init_main()`):
the entry point is a readable checklist, and each subsystem owns its own implementation
details behind a single `_init()` call. As Phase 1 continues, this function will grow by
one line per subsystem — never by inlined logic.

### 3.2 Why the GDT gets rebuilt here, not left as boot.S's version

`boot.S`'s GDT was explicitly transitional (this is documented in its own bootloader docs):
it exists only to survive the far jump into long mode and has just two segments, hand-
encoded as raw hex. It has no concept of user-mode (ring 3) segments, and editing it means
hand-computing descriptor bit patterns in assembly with no type checking.

`gdt.c` replaces it with:
- A typed C struct (`gdt_entry_t`) so the byte layout is visible and named, not opaque hex
- 5 segments instead of 2: null, kernel code, kernel data, user code, user data
- Named selector indices (`GDT_SEGMENT_KERNEL_CODE` etc.) that later subsystems will
  reference instead of hardcoding magic numbers like `0x08`

The swap happens via `gdt_flush()` (necessarily assembly — there's no C syntax for reloading
the CS register) and is invoked once, early in `kmain()`. We verified empirically that
execution continues correctly immediately after this swap (the next `kprintf` call
succeeds), which is the real risk here: a malformed new GDT triple-faults the instant it's
loaded, with the failure happening *inside* `lgdt`/the far-return trick, before any
diagnostic could print.

**Why user-mode segments are added now, even though nothing uses them:** this is a
deliberate, narrow exception to "don't write code nothing calls yet." The GDT's *layout* is
structural — once `syscall`/`process` subsystems start referencing segment selectors by
index, changing that layout becomes a breaking change across multiple subsystems. Settling
it once now, while it's cheap and we're already touching this file, avoids a future
migration. This reasoning does NOT extend to behavior — we added data (two struct entries),
not logic; no function exists yet that uses ring 3.

### 3.3 The freestanding library (`kernel/lib/`)

Three small files, added because the C compiler itself generates implicit calls to
`memset`/`memcpy` for certain patterns (zeroing structs, struct assignment) regardless of
whether the kernel author calls them explicitly — without our own implementations, code that
looks like it shouldn't need libc would fail to link with "undefined reference to memset."

- `kstring.c` — `memset`, `memcpy`, `memcmp`, `strlen`. Deliberately simple byte-at-a-time
  implementations; optimizing these is explicitly out of scope until correctness-first
  versions exist and profiling (much later) shows it matters.
- `port_io.c` — `outb`/`inb`, factored out of the bootloader stub's inline code. The first
  new consumer beyond `kio.c` will be the Keyboard Driver subsystem (`inb` from port 0x60).

### 3.4 kio.c: seed of the future Text Output driver, not the driver itself

This is worth being precise about, since it's easy to conflate with Phase 1, Feature 3
(Text Output) later: `kio.c` exists so the *kernel's own* boot diagnostics have one
non-duplicated home, with cursor tracking and scrolling so multi-line boot logs don't
overwrite each other on screen. It is explicitly NOT:
- A general-purpose driver API for applications to print to the screen
- Color-capable, beyond the single fixed white-on-black scheme
- The final word on how AcharyaOS handles text output

The future Text Output subsystem will likely subsume or significantly extend this module.
We're flagging the boundary now so it doesn't quietly become "the" text driver by default
inertia.

`kprintf`'s format support is intentionally minimal: `%s %d %x %c %%`. This is not a libc
printf clone, and expanding it (e.g. `%p`, width specifiers, `%lld`) is in scope for later
work, not this feature.

## 4. Folder Structure

```
AcharyaOS/
├── boot/arch/x86_64/          # unchanged from Feature 1
│   ├── boot.S
│   └── linker.ld
├── kernel/
│   ├── kmain.c                  # REWRITTEN: staged init orchestrator
│   ├── arch/x86_64/
│   │   ├── gdt.c                 # NEW: kernel-owned GDT as typed C struct
│   │   └── gdt_flush.S            # NEW: lgdt + segment reload (asm-mandatory part)
│   ├── lib/
│   │   ├── port_io.c               # NEW: outb/inb, factored out of bootloader stub
│   │   ├── kstring.c                # NEW: freestanding memset/memcpy/memcmp/strlen
│   │   └── kio.c                     # NEW: kprintf/kputs, VGA+serial backend
│   └── include/
│       ├── kernel.h                   # NEW: shared macros (NORETURN, PACKED), kernel_panic decl
│       ├── gdt.h
│       ├── kio.h
│       ├── kstring.h
│       └── port_io.h
├── tools/grub.cfg               # unchanged
├── docs/
│   ├── 01-bootloader.md         # unchanged
│   └── 02-kernel.md              # this file
└── Makefile                      # CHANGED: wildcard-based source discovery (see below)
```

## 5. Build System Change Worth Knowing About

The Makefile switched from explicitly listing every source file to discovering them via
`find $(KERNEL_DIR) -name '*.c'` / `'*.S'`. This means **future subsystems do not need to
edit the Makefile** to get their files built — drop a `.c` or `.S` file anywhere under
`kernel/`, and the next `make` picks it up automatically via generic pattern rules.

**One naming rule this introduced, documented inline in the Makefile:** a `.c` file and a
`.S` file with the same basename (e.g. `gdt.c` + `gdt.S`) both compile to the same object
path (`build/.../gdt.o`), silently colliding — one object overwrites the other in the link
list with no warning. We hit this exact bug while building this feature (see Section 7) and
fixed it by renaming the asm helper to `gdt_flush.S`. The rule going forward: when a module
needs both a `.c` file and a small asm helper, give the asm file a distinct,
purpose-describing name.

## 6. Build & Test Instructions

Unchanged from Feature 1:
```bash
make clean && make iso     # build the bootable ISO
make run                    # interactive QEMU run (serial as stdio)
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
------------------------------------------------
AcharyaOS kernel initialization complete.
(No further subsystems are implemented yet - this is the
 end of Phase 1, Feature 2. Halting.)
```

### Validating Multiboot2 compliance directly (new technique introduced this feature)
```bash
grub-file --is-x86-multiboot2 build/acharyaos.bin
echo $?   # 0 = valid, nonzero = invalid
```
This is GRUB's own authoritative checker — more reliable than eyeballing `readelf -S`
output, which can look concerning (see Section 7) even when the binary is fully correct,
because GRUB loads Multiboot2 kernels by scanning raw file bytes for the magic number, not
via ELF program headers.

## 7. Verification Evidence

```
$ make clean && make all
... (6 source files compile with ZERO warnings beyond the two known/documented
     linker warnings carried over from Feature 1: missing .note.GNU-stack,
     RWX LOAD segment - both pre-existing, both explained in 01-bootloader.md) ...
Built kernel binary: build/acharyaos.bin

$ grub-file --is-x86-multiboot2 build/acharyaos.bin; echo $?
0

$ qemu-system-x86_64 -cdrom build/acharyaos.iso -m 256M -display none \
    -serial file:build/serial.log -no-reboot
$ cat build/serial.log
AcharyaOS kernel starting...
------------------------------------------------
[init] Loading kernel GDT... done.
------------------------------------------------
AcharyaOS kernel initialization complete.
(No further subsystems are implemented yet - this is the
 end of Phase 1, Feature 2. Halting.)
```

The "done." printing *after* `gdt_init()` returns is the key evidence here: it proves
execution survived the GDT swap (a bad descriptor or a broken `gdt_flush` would triple-fault
inside that call, before this line could ever print).

### A real bug caught and fixed during this feature (worth keeping in the record)
Initial build silently linked `gdt.o` twice (once compiled from `gdt.c`, once from a
same-named `gdt.S`) due to a Makefile pattern-rule collision — both files mapped to
`build/arch/x86_64/gdt.o`. This produced a "multiple definition of gdt_init" / "undefined
reference to gdt_flush" pair of linker errors. Fixed by renaming the asm file to
`gdt_flush.S` and documenting the naming rule in the Makefile so it isn't rediscovered the
hard way by a future subsystem (the IDT work is the most likely place this would recur,
since `idt.c` + `idt.S` is an equally natural pairing).

## 8. Debugging Strategy (additions specific to this feature)

### 8.1 "It built, but crashes/reboots after the GDT message"
This means `gdt_flush` itself triple-faulted. Check, in order:
1. Does `gdt_pointer.limit` equal `sizeof(gdt_entries) - 1`, not `sizeof(gdt_entries)`?
   (Off-by-one here is a classic mistake — the GDT limit field is the last valid byte
   offset, not the total size.)
2. Are the selector values used in the `mov $0x10, %ax` / `push $0x08` lines in
   `gdt_flush.S` actually `index * 8` for the segments you intend? (0x08 = index 1 = kernel
   code; 0x10 = index 2 = kernel data, matching `GDT_SEGMENT_KERNEL_CODE`/`_DATA` in `gdt.c`.)
   If you reorder `gdt_entries[]` in `gdt.c` without updating these literals in the `.S`
   file, you get a silent, very confusing mismatch — this is the sharpest edge in this
   subsystem and worth a comment-level warning at both ends (already added).
3. Use the GDB-over-QEMU technique from the bootloader's debugging guide
   (`qemu-system-x86_64 ... -s -S`, then `target remote localhost:1234`, `stepi` through
   `gdt_flush` instruction-by-instruction) if 1 and 2 don't reveal the issue.

### 8.2 "kprintf prints garbage / wrong numbers"
Check the format specifier against the actual argument type passed — there is currently NO
type checking between `kprintf`'s `%d`/`%x`/etc and what you pass in, because we don't have
GCC's `__attribute__((format(printf, ...)))` wired up yet (that attribute assumes libc
printf's exact semantics, which ours deliberately doesn't fully match - e.g. our `%d` is
hardcoded to 32-bit, not following `l`/`ll` length modifiers). Passing a 64-bit value where
`%d`/`%x` expects 32-bit will silently misread the variadic arguments. This is a known
sharp edge to revisit if/when this module gets a length-modifier upgrade.

### 8.3 New build-system failure mode: "my new file isn't being compiled"
With the wildcard-based Makefile, this now almost always means either (a) the file isn't
actually under `kernel/` (check the path), or (b) it collides on basename with another `.c`/
`.S` pair per the naming rule in Section 5. Run `make -p | grep KERNEL_C_SRCS` /
`KERNEL_S_SRCS` to see exactly what `find` discovered, before assuming the bug is elsewhere.

## 9. Known Limitations (intentional, to be addressed by later subsystems)

- **No IDT yet** — any CPU exception (divide-by-zero, page fault, general protection fault)
  is still a silent triple-fault with zero diagnostic output. This is the single biggest
  debugging handicap right now and is the explicit focus of the upcoming Timer Interrupts
  subsystem.
- **Multiboot2 info pointer is accepted but unparsed** — `kmain()` receives it correctly
  (calling convention honored) but does nothing with it yet. The memory map it contains is
  needed by the Memory Allocator / Virtual Memory subsystems.
- **User-mode GDT segments are defined but never loaded into any running context** — no
  ring-3 code exists yet; that's the User-Space Programs feature, much later in Phase 1.
- **kprintf has no length modifiers** (`%ld`, `%lld`) and silently mishandles 64-bit
  arguments passed to `%d`/`%x` — flagged above in the debugging guide rather than fixed
  now, since fixing it properly means deciding the module's long-term format-string contract,
  which is better done once we know what subsystems actually need from it.
