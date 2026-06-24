/*
 * AcharyaOS - scheduler.c
 * -----------------------
 * Early round-robin scheduler accounting. The timer interrupt calls
 * scheduler_on_timer_tick(), which charges time to the current task and
 * advances to the next runnable task when the current task's time slice ends.
 *
 * This is intentionally not yet a context switcher. Context switching needs
 * saved register state, kernel stacks, and process/thread ownership, which are
 * the next subsystem's job. This module gives that future work a clean policy
 * layer instead of mixing scheduling decisions into interrupt code.
 */

#include "scheduler.h"

static sched_task_info_t tasks[SCHED_MAX_TASKS];
static size_t task_count;
static size_t current_index;
static uint64_t total_ticks;
static uint32_t next_task_id;
static uint32_t slice_progress;

static size_t runnable_count_now(void) {
    size_t count = 0;
    for (size_t i = 0; i < task_count; i++) {
        if (tasks[i].state == SCHED_TASK_RUNNABLE) {
            count++;
        }
    }
    return count;
}

static void advance_to_next_runnable(void) {
    if (task_count == 0) {
        return;
    }

    for (size_t step = 0; step < task_count; step++) {
        current_index = (current_index + 1) % task_count;
        if (tasks[current_index].state == SCHED_TASK_RUNNABLE) {
            slice_progress = 0;
            return;
        }
    }
}

void scheduler_init(void) {
    for (size_t i = 0; i < SCHED_MAX_TASKS; i++) {
        tasks[i].id = 0;
        tasks[i].name = 0;
        tasks[i].state = SCHED_TASK_UNUSED;
        tasks[i].ticks_seen = 0;
        tasks[i].time_slice_ticks = 0;
    }

    task_count = 0;
    current_index = 0;
    total_ticks = 0;
    next_task_id = 1;
    slice_progress = 0;
}

int scheduler_register_task(const char *name, uint32_t time_slice_ticks) {
    if (task_count >= SCHED_MAX_TASKS) {
        return -1;
    }
    if (time_slice_ticks == 0) {
        time_slice_ticks = 1;
    }

    sched_task_info_t *task = &tasks[task_count];
    task->id = next_task_id++;
    task->name = name;
    task->state = SCHED_TASK_RUNNABLE;
    task->ticks_seen = 0;
    task->time_slice_ticks = time_slice_ticks;

    task_count++;
    return (int) task->id;
}

void scheduler_on_timer_tick(void) {
    total_ticks++;

    if (task_count == 0 || runnable_count_now() == 0) {
        return;
    }

    if (tasks[current_index].state != SCHED_TASK_RUNNABLE) {
        advance_to_next_runnable();
    }

    tasks[current_index].ticks_seen++;
    slice_progress++;

    if (slice_progress >= tasks[current_index].time_slice_ticks) {
        advance_to_next_runnable();
    }
}

void scheduler_get_stats(sched_stats_t *stats) {
    if (!stats) {
        return;
    }

    stats->total_ticks = total_ticks;
    stats->current_task_id = task_count > 0 ? tasks[current_index].id : 0;
    stats->task_count = task_count;
    stats->runnable_count = runnable_count_now();
}

size_t scheduler_copy_tasks(sched_task_info_t *out, size_t max_tasks) {
    if (!out) {
        return 0;
    }

    size_t count = task_count < max_tasks ? task_count : max_tasks;
    for (size_t i = 0; i < count; i++) {
        out[i] = tasks[i];
    }
    return count;
}
