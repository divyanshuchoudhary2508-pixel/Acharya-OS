# AcharyaOS - Root Makefile
# ===========================
# This Makefile currently builds Phase 1, Feature 1 only: the bootloader
# and a minimal stub kernel needed to verify the boot handoff. As each new
# subsystem is added, this file will grow - new subsystems contribute their
# own object files into KERNEL_OBJS rather than each subsystem getting its
# own ad-hoc build process. This keeps the whole OS as ONE coherent binary
# with ONE build graph, which is how real kernels (Linux, BSD, etc) do it.

# --- Toolchain ---------------------------------------------------------
AS       := nasm                     # not actually used yet (we use GNU as via gcc for .S), kept for future NASM modules
CC       := gcc
LD       := ld
GRUB_MKRESCUE := grub-mkrescue

# --- Directories --------------------------------------------------------
BOOT_DIR    := boot/arch/x86_64
KERNEL_DIR  := kernel
BUILD_DIR   := build
ISO_DIR     := $(BUILD_DIR)/iso

# SRC_DIRS: every top-level folder whose .c/.S files get compiled into the
# final kernel binary. Phase 1, Feature 3 (Text Output) is the first
# subsystem to live OUTSIDE kernel/ (it lives in drivers/, per the
# project's required top-level structure) - so this list grows from one
# entry to two here. As later subsystems populate memory/, process/,
# scheduler/, fs/, etc, each just gets appended to this list - no other
# Makefile change needed.
SRC_DIRS    := kernel drivers memory scheduler process syscall fs users gui

# --- Flags ---------------------------------------------------------
# -ffreestanding : we are NOT building a normal hosted program. No libc,
#                  no startup files, no assumption of an OS underneath us.
# -fno-stack-protector : stack-protector support needs runtime helpers
#                  (__stack_chk_fail) that don't exist in a freestanding
#                  binary - omit it for now (we'll revisit stack safety
#                  properly in a later hardening pass).
# -mno-red-zone  : the x86_64 "red zone" (128 bytes below RSP usable
#                  without adjusting RSP) is unsafe in interrupt/exception
#                  handlers because the CPU itself can write below RSP on
#                  interrupt entry, corrupting it. Since this rule is
#                  easy to forget once we add real interrupt handlers
#                  later, we disable it globally now rather than risk a
#                  subtle bug after the fact.
# -mcmodel=kernel: tells GCC to generate code assuming we live in the
#                  negative/high address space typical of kernels (also
#                  affects how it accesses static data - safer default
#                  for freestanding kernel code than the default model).
# -Wall -Wextra  : we want every warning GCC can give us. In kernel code,
#                  a warning today is a triple-fault tomorrow.
CFLAGS := -ffreestanding -fno-stack-protector -mno-red-zone \
          -mcmodel=kernel -fno-pic -fno-pie -Wall -Wextra -std=gnu11 -g -O0 \
          $(addprefix -I,$(addsuffix /include,$(SRC_DIRS)))

# Assembler flags for our .S file (compiled via gcc, which invokes the
# GNU assembler `as` under the hood - this lets us use the C preprocessor
# in assembly files if we ever want to, and keeps one consistent toolchain).
ASFLAGS := -ffreestanding

# Linker flags:
# -n            : disable page alignment of sections in the output file
#                 (we control alignment explicitly in linker.ld instead)
# -T linker.ld  : use OUR linker script, not the system default
# -static       : no dynamic linking - makes no sense for a kernel
LDFLAGS := -n -T $(BOOT_DIR)/linker.ld -static

# --- Source files --------------------------------------------------------
# As of Phase 1 Feature 2, the kernel has grown past "one file" - it now
# has kernel/, kernel/arch/x86_64/, and kernel/lib/. Rather than hand-list
# every .c and .S file (which we'd have to remember to update every single
# time a new subsystem adds a file - exactly the kind of maintenance trap
# that causes "why isn't my new file being compiled" bugs), we use `find`
# to discover sources automatically. boot.S is kept separate and explicit
# since it is architecturally special: it's the ONLY file that must be
# first in link order (it contains _start and the Multiboot2 header, which
# must appear near the start of the binary).
#
# NAMING RULE - read before adding a new .S file:
#   A .c file and a .S file with the SAME basename (e.g. gdt.c + gdt.S)
#   both compile to the same object path (build/.../gdt.o), silently
#   colliding - one overwrites the other in the object list with no
#   warning from `find`, and the resulting link error (duplicate symbol /
#   missing symbol) does not obviously point back to this cause. When a
#   module needs both a .c file and a small asm helper, give the asm file
#   a distinct, purpose-describing name instead (e.g. gdt.c + gdt_flush.S,
#   NOT gdt.c + gdt.S).
BOOT_SRC     := $(BOOT_DIR)/boot.S
KERNEL_C_SRCS := $(shell find $(SRC_DIRS) -name '*.c')
KERNEL_S_SRCS := $(shell find $(SRC_DIRS) -name '*.S')

BOOT_OBJ     := $(BUILD_DIR)/boot.o
# Object files are placed at build/<original/path/from/repo/root>.o - e.g.
# kernel/lib/kio.c -> build/kernel/lib/kio.o, drivers/vga/vga.c ->
# build/drivers/vga/vga.o. Mirroring the FULL path (not just the part
# after kernel/ or drivers/) is what lets two different top-level source
# directories safely contain files with the same basename without their
# object files colliding - e.g. kernel/foo.c and drivers/foo.c would
# previously both have mapped to build/foo.o.
KERNEL_C_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRCS))
KERNEL_S_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(KERNEL_S_SRCS))

# boot.o is listed FIRST so _start lands at the very beginning of .text,
# satisfying ENTRY(_start) and the Multiboot2 32KB-header placement rule.
ALL_OBJS    := $(BOOT_OBJ) $(KERNEL_C_OBJS) $(KERNEL_S_OBJS)

KERNEL_BIN  := $(BUILD_DIR)/acharyaos.bin
ISO_FILE    := $(BUILD_DIR)/acharyaos.iso

# --- Top-level targets --------------------------------------------------------

.PHONY: all clean run run-serial iso

all: $(KERNEL_BIN)

# Build the raw kernel binary (boot.S + all kernel/ sources linked together).
$(KERNEL_BIN): $(ALL_OBJS) $(BOOT_DIR)/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo "Built kernel binary: $@"

$(BUILD_DIR)/boot.o: $(BOOT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@

# Generic pattern rules: any <srcdir>/**/*.c -> build/<srcdir>/**/*.o, same
# for .S. $(dir $@) handles subdirectories (e.g. drivers/vga/vga.c needs
# build/drivers/vga/ to exist before the compiler can write its .o there).
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build a bootable ISO image using GRUB. This is what real hardware (or
# QEMU acting as real hardware) actually boots from - GRUB is embedded
# into the ISO's boot sector, reads grub.cfg, and loads our kernel binary
# per the Multiboot2 protocol.
iso: $(KERNEL_BIN)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/acharyaos.bin
	cp tools/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO_FILE) $(ISO_DIR) 2>&1 | tail -5
	@echo "Built bootable ISO: $(ISO_FILE)"

# Run in QEMU with a graphical-style display captured to a file (for
# environments without a real display) plus serial output to stdio.
run: iso
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -display none \
		-serial mon:stdio -no-reboot -no-shutdown

# Run in QEMU and dump the VGA text-mode screen contents to a file, then
# exit - useful for headless verification in CI / containers with no GUI.
run-headless-check: iso
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -display none \
		-no-reboot -monitor unix:$(BUILD_DIR)/qemu-mon.sock,server,nowait &
	sleep 2
	echo '{"execute":"qmp_capabilities"}{"execute":"human-monitor-command","arguments":{"command-line":"screendump $(BUILD_DIR)/screen.ppm"}}' | \
		timeout 2 socat - unix-connect:$(BUILD_DIR)/qemu-mon.sock > /dev/null 2>&1 || true
	pkill -f "qemu-system-x86_64.*$(ISO_FILE)" || true

clean:
	rm -rf $(BUILD_DIR)
