/*
 * AcharyaOS - process.c
 * ---------------------
 * Early process table. There is no userspace, ELF loader, or saved CPU
 * context yet, so a "process" is currently metadata plus a scheduler binding.
 * That may sound small, but it creates the ownership boundary the next steps
 * need: process management owns PIDs and states; the scheduler owns time.
 */

#include "process.h"
#include "scheduler.h"
#include "timer.h"

static process_info_t processes[PROCESS_MAX];
static size_t process_count;
static uint32_t next_pid;
static uint32_t current_pid;

static size_t ready_count_now(void) {
    size_t count = 0;
    for (size_t i = 0; i < process_count; i++) {
        if (processes[i].state == PROCESS_READY ||
            processes[i].state == PROCESS_RUNNING) {
            count++;
        }
    }
    return count;
}

void process_init(void) {
    for (size_t i = 0; i < PROCESS_MAX; i++) {
        processes[i].pid = 0;
        processes[i].scheduler_task_id = 0;
        processes[i].name = 0;
        processes[i].state = PROCESS_UNUSED;
        processes[i].created_at_tick = 0;
    }

    process_count = 0;
    next_pid = 1;
    current_pid = 0;

    process_create_kernel("kernel-shell", 5);
    process_create_kernel("idle", 5);
    current_pid = 1;
}

int process_create_kernel(const char *name, uint32_t time_slice_ticks) {
    if (process_count >= PROCESS_MAX) {
        return -1;
    }

    int scheduler_task_id = scheduler_register_task(name, time_slice_ticks);
    if (scheduler_task_id < 0) {
        return -1;
    }

    process_info_t *process = &processes[process_count];
    process->pid = next_pid++;
    process->scheduler_task_id = (uint32_t) scheduler_task_id;
    process->name = name;
    process->state = process_count == 0 ? PROCESS_RUNNING : PROCESS_READY;
    process->created_at_tick = timer_get_ticks();

    process_count++;
    return (int) process->pid;
}

void process_get_stats(process_stats_t *stats) {
    if (!stats) {
        return;
    }

    stats->process_count = process_count;
    stats->ready_count = ready_count_now();
    stats->next_pid = next_pid;
}

uint32_t process_current_pid(void) {
    return current_pid;
}

size_t process_get_count(void) {
    return process_count;
}

size_t process_copy_table(process_info_t *out, size_t max_processes) {
    if (!out) {
        return 0;
    }

    size_t count = process_count < max_processes ? process_count : max_processes;
    for (size_t i = 0; i < count; i++) {
        out[i] = processes[i];
    }
    return count;
}

const char *process_state_name(process_state_t state) {
    switch (state) {
        case PROCESS_UNUSED:  return "unused";
        case PROCESS_READY:   return "ready";
        case PROCESS_RUNNING: return "running";
        case PROCESS_BLOCKED: return "blocked";
        case PROCESS_ZOMBIE:  return "zombie";
        default:              return "unknown";
    }
}
