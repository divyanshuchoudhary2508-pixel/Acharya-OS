/*
 * AcharyaOS - kmain.c
 * --------------------
 * This file is now the REAL kernel entry point (Phase 1, Feature 2),
 * replacing the bootloader-verification stub from Feature 1.
 *
 * kmain()'s job is to be an ORCHESTRATOR, not a place where logic lives.
 * Each subsystem gets its own xxx_init() function, called here in a fixed,
 * documented order. This mirrors how real kernels (Linux's start_kernel(),
 * BSD's init_main(), etc) are structured - the entry point reads like a
 * checklist, and each item is a single function call into a subsystem that
 * owns its own implementation details.
 *
 * WHY THIS ORDER MATTERS:
 *   1. kio_init()  - must be first. Every subsequent step wants to be able
 *      to print its own progress/failure. Without this, an early failure
 *      in any later step would be silent and undebuggable. As of Feature 3,
 *      kio_init() itself now delegates VGA setup to drivers/vga/vga_init()
 *      rather than owning that hardware knowledge directly.
 *   2. gdt_init()  - replaces boot.S's throwaway GDT with the kernel's
 *      permanent one. Must happen before anything that might rely on
 *      specific segment selector values (nothing does yet, but interrupt
 *      handling - added next - will, since IDT entries reference code
 *      segment selectors).
 *   3. (Feature 3, Text Output, has no separate init step of its own here -
 *      it is fully brought up as part of step 1's kio_init(), since output
 *      capability and "the text driver is ready" are the same milestone.
 *      The color demonstration below is just proof, not a new init call.)
 *   (Future steps - idt_init(), memory_init(), etc - will be added here as
 *   their respective subsystems are built, each preceded by the same
 *   "why this position in the order" reasoning.)
 */

#include "kernel.h"
#include "kio.h"
#include "gdt.h"
#include "vga.h"
#include "pic.h"
#include "idt.h"
#include "keyboard.h"
#include "timer.h"
#include "shell.h"
#include "kheap.h"
#include "vmm.h"
#include "scheduler.h"
#include "process.h"
#include "syscall.h"
#include "ata.h"
#include "fs.h"
#include "userspace.h"
#include "log.h"
#include "config.h"
#include "package.h"
#include "service.h"
#include "user.h"
#include "framebuffer.h"
#include "window.h"
#include "mouse.h"
#include "button.h"

/*
 * kernel_panic: prints a fatal error and halts the CPU forever.
 * Declared in kernel.h (so any subsystem can call it without including
 * kio.h directly), implemented here because kmain.c is the natural home
 * for "what does the kernel do when it cannot continue" - this is a
 * kernel-wide policy decision, not something any one subsystem should
 * decide independently.
 */
void kernel_panic(const char *message) {
    log_write(LOG_PANIC, message);
    kprintf("\n[PANIC] %s\n[PANIC] System halted.\n", message);
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

static void boot_readline(char *buffer, int max_len) {
    int len = 0;
    for (;;) {
        int ch = keyboard_getchar();
        if (ch < 0) {
            __asm__ volatile ("hlt");
            continue;
        }
        if (ch == '\n') {
            kprintf("\n");
            buffer[len] = '\0';
            return;
        }
        if (ch == '\b') {
            if (len > 0) {
                len--;
                kprintf("\b");
            }
            continue;
        }
        if (ch >= 32 && ch <= 126 && len < max_len - 1) {
            buffer[len++] = (char) ch;
            kprintf("%c", ch);
        }
    }
}

static void boot_prompt_login(void) {
    char username[USER_NAME_MAX];
    char password[USER_PASS_MAX];

    for (;;) {
        kprintf("[login] username: ");
        boot_readline(username, sizeof(username));
        if (username[0] == '\0') {
            continue;
        }
        kprintf("[login] password: ");
        boot_readline(password, sizeof(password));
        if (user_login(username, password) == 0) {
            kprintf("[login] welcome, %s\n", username);
            return;
        }
        kprintf("[login] invalid credentials\n");
    }
}

/*
 * kmain - kernel entry point, called from boot.S.
 *
 * multiboot_info_ptr: physical address of the Multiboot2 info structure,
 * passed in RDI by boot.S per the x86_64 System V calling convention.
 * We accept it now (honoring the calling convention) but don't parse its
 * contents yet - that belongs to the memory map portion of the upcoming
 * Memory Allocator / Virtual Memory subsystems, which need the BIOS/GRUB-
 * reported memory map this structure contains.
 */
void kmain(uint64_t multiboot_info_ptr) {
    (void) multiboot_info_ptr;

    /* Step 1: bring up output FIRST so every later step can report itself. */
    kio_init();
    log_init();
    log_write(LOG_INFO, "AcharyaOS kernel starting");
    kprintf("------------------------------------------------\n");

    /* Step 2: install the kernel's permanent GDT, replacing boot.S's
       transitional one. */
    log_write(LOG_INFO, "Loading kernel GDT");
    kprintf("[init] Loading kernel GDT... ");
    gdt_init();
    kprintf("done.\n");

    /* Step 3: demonstrate the Text Output subsystem (Phase 1, Feature 3)
       is real - not just "compiles," but actually drives color and the
       hardware cursor through the new drivers/vga/ module. This is a
       deliberately visible self-test: if vga.c's color math is wrong,
       this is where it would show up as garbled or wrong-colored text. */
    log_write(LOG_DEBUG, "Text Output driver active");
    kprintf("[init] Text Output driver (drivers/vga) active.\n");
    kio_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("[ ok ] ");
    kio_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("Color output verified (this line printed in default grey).\n");

    /* Step 4: install interrupt infrastructure and the first hardware
       interrupt consumer: the PS/2 keyboard driver. The order is important:
       the PIC is remapped while interrupts are still disabled, the IDT is
       loaded before any IRQ can arrive, and the keyboard unmasks only IRQ1
       after its handler is registered. */
    log_write(LOG_INFO, "Remapping legacy PIC");
    kprintf("[init] Remapping legacy PIC... ");
    pic_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Loading IDT");
    kprintf("[init] Loading IDT... ");
    idt_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting keyboard driver");
    kprintf("[init] Starting keyboard driver... ");
    keyboard_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting framebuffer graphics core");
    kprintf("[init] Starting framebuffer graphics core... ");
    fb_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting window manager");
    kprintf("[init] Starting window manager... ");
    wm_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting mouse driver");
    kprintf("[init] Starting mouse driver... ");
    mouse_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting button control layer");
    kprintf("[init] Starting button control layer... ");
    btn_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting PIT timer at 100 Hz");
    kprintf("[init] Starting PIT timer at 100 Hz... ");
    timer_init(100);
    kprintf("done.\n");

    log_write(LOG_INFO, "Installing kernel-owned page tables");
    kprintf("[init] Installing kernel-owned page tables... ");
    vmm_init();
    if (!vmm_self_test()) {
        kernel_panic("Virtual memory self-test failed");
    }
    kprintf("done.\n");

    /* Step 5: initialize the early heap before the shell starts accepting
       commands. The current heap is a simple bump allocator; it gives later
       kernel subsystems a clean kmalloc/kzalloc API while we defer real
       freeing and page-backed allocation until Virtual Memory. */
    log_write(LOG_INFO, "Starting early kernel heap");
    kprintf("[init] Starting early kernel heap... ");
    kheap_init();
    if (!kheap_self_test()) {
        kernel_panic("Early heap self-test failed");
    }
    kprintf("done.\n");

    log_write(LOG_INFO, "Starting round-robin scheduler core");
    kprintf("[init] Starting round-robin scheduler core... ");
    scheduler_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Creating initial kernel processes");
    kprintf("[init] Creating initial kernel processes... ");
    process_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Installing syscall interface");
    kprintf("[init] Installing syscall interface... ");
    syscall_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Probing ATA disk");
    kprintf("[init] Probing ATA disk... ");
    ata_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Mounting filesystem");
    kprintf("[init] Mounting filesystem... ");
    fs_init();
    kprintf("done.\n");

    log_write(LOG_INFO, "Loading configuration");
    kprintf("[init] Loading configuration... ");
    config_init();
    {
        char level[16];
        if (config_get("loglevel", level, sizeof(level)) == 0) {
            if (strcmp(level, "trace") == 0) {
                log_set_minimum_level(LOG_TRACE);
            } else if (strcmp(level, "debug") == 0) {
                log_set_minimum_level(LOG_DEBUG);
            } else if (strcmp(level, "info") == 0) {
                log_set_minimum_level(LOG_INFO);
            } else if (strcmp(level, "warn") == 0) {
                log_set_minimum_level(LOG_WARN);
            } else if (strcmp(level, "error") == 0) {
                log_set_minimum_level(LOG_ERROR);
            } else if (strcmp(level, "panic") == 0) {
                log_set_minimum_level(LOG_PANIC);
            }
        }
    }
    kprintf("done.\n");

    log_write(LOG_INFO, "Initializing startup services");
    kprintf("[init] Initializing startup services... ");
    service_init();
    service_startup_all();
    kprintf("done.\n");

    log_write(LOG_INFO, "Initializing user accounts");
    kprintf("[init] Initializing user accounts... ");
    user_init();
    user_create_default_passwords();
    kprintf("done.\n");

    log_write(LOG_INFO, "Preparing user-space demo");
    kprintf("[init] Preparing user-space demo... ");
    package_init();
    userspace_init();
    kprintf("done.\n");

    __asm__ volatile ("sti");

    boot_prompt_login();

    kprintf("------------------------------------------------\n");
    kprintf("AcharyaOS kernel initialization complete.\n");
    kprintf("(Phase 3, Feature 28 - Mouse Driver - now active.)\n\n");
    log_write(LOG_INFO, "Kernel initialization complete");

    shell_run();
}
