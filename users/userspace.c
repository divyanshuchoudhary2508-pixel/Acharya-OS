/*
 * AcharyaOS - userspace.c
 * -----------------------
 * Very small ring-3 demo launcher. It copies a prebuilt user program blob and
 * switches into it with iretq.
 */

#include "userspace.h"
#include "vmm.h"
#include "kio.h"
#include "kernel.h"
#include "kstring.h"

extern uint8_t demo_user_program_start;
extern uint8_t demo_user_program_end;

#define USER_CODE_VADDR  0x8000000000ull
#define USER_STACK_VADDR  0x8000200000ull
#define USER_STACK_TOP   (USER_STACK_VADDR + VMM_PAGE_SIZE_2M)
#define USER_RING3_CS    0x1B
#define USER_RING3_DS    0x23

static uint8_t user_code_page[VMM_PAGE_SIZE_2M] __attribute__((aligned(2097152)));
static uint8_t user_stack_page[VMM_PAGE_SIZE_2M] __attribute__((aligned(2097152)));

static volatile int userspace_running = 0;

__attribute__((noreturn))
static void userspace_enter(uint64_t entry, uint64_t stack_top) {
    __asm__ volatile (
        "movw %[ds], %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "xor %%eax, %%eax\n\t"
        "movw %[ss], %%ax\n\t"
        "pushq %%rax\n\t"
        "pushq %[stack]\n\t"
        "pushfq\n\t"
        "pushq %[cs]\n\t"
        "pushq %[entry]\n\t"
        "iretq\n\t"
        :
        : [ds] "i" (USER_RING3_DS),
          [ss] "i" (USER_RING3_DS),
          [cs] "r" ((uint64_t) USER_RING3_CS),
          [stack] "r" (stack_top),
          [entry] "r" (entry)
        : "rax", "memory"
    );
    __builtin_unreachable();
}

void userspace_init(void) {
    size_t code_size = (size_t) (&demo_user_program_end - &demo_user_program_start);
    if (code_size > sizeof(user_code_page)) {
        kernel_panic("User program image too large");
    }

    memset(user_code_page, 0, sizeof(user_code_page));
    memset(user_stack_page, 0, sizeof(user_stack_page));
    memcpy(user_code_page, &demo_user_program_start, code_size);

    if (vmm_map_user_2m(USER_CODE_VADDR, (uintptr_t) user_code_page) != 0) {
        kernel_panic("Failed to map user code page");
    }
    if (vmm_map_user_2m(USER_STACK_VADDR, (uintptr_t) user_stack_page) != 0) {
        kernel_panic("Failed to map user stack page");
    }

    userspace_running = 0;
}

void userspace_run_demo(void) {
    if (userspace_running) {
        kprintf("user demo already running\n");
        return;
    }
    userspace_running = 1;
    kprintf("entering user mode demo...\n");
    userspace_enter(USER_CODE_VADDR, USER_STACK_TOP);
}
