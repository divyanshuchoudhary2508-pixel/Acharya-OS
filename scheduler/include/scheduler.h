/*
 * AcharyaOS - scheduler.h
 * -----------------------
 * Phase 1, Feature 9: round-robin scheduler core. This first version tracks
 * runnable kernel tasks and advances scheduling decisions on timer ticks.
 * Actual saved CPU contexts arrive with Process Management.
 */

#ifndef ACHARYAOS_SCHEDULER_H
#define ACHARYAOS_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#define SCHED_MAX_TASKS 8

typedef enum {
    SCHED_TASK_UNUSED = 0,
    SCHED_TASK_RUNNABLE,
    SCHED_TASK_BLOCKED,
} sched_task_state_t;

typedef struct {
    uint32_t id;
    const char *name;
    sched_task_state_t state;
    uint64_t ticks_seen;
    uint32_t time_slice_ticks;
} sched_task_info_t;

typedef struct {
    uint64_t total_ticks;
    uint32_t current_task_id;
    size_t task_count;
    size_t runnable_count;
} sched_stats_t;

void scheduler_init(void);
int scheduler_register_task(const char *name, uint32_t time_slice_ticks);
void scheduler_on_timer_tick(void);
void scheduler_get_stats(sched_stats_t *stats);
size_t scheduler_copy_tasks(sched_task_info_t *out, size_t max_tasks);

#endif /* ACHARYAOS_SCHEDULER_H */
