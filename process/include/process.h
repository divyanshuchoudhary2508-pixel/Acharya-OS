/*
 * AcharyaOS - process.h
 * ---------------------
 * Phase 1, Feature 10: process management. A process is the kernel's record
 * of an executable thing: pid, name, state, and scheduler task binding.
 */

#ifndef ACHARYAOS_PROCESS_H
#define ACHARYAOS_PROCESS_H

#include <stddef.h>
#include <stdint.h>

#define PROCESS_MAX 16

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE,
} process_state_t;

typedef struct {
    uint32_t pid;
    uint32_t scheduler_task_id;
    const char *name;
    process_state_t state;
    uint64_t created_at_tick;
} process_info_t;

typedef struct {
    size_t process_count;
    size_t ready_count;
    uint32_t next_pid;
} process_stats_t;

void process_init(void);
int process_create_kernel(const char *name, uint32_t time_slice_ticks);
void process_get_stats(process_stats_t *stats);
size_t process_copy_table(process_info_t *out, size_t max_processes);
const char *process_state_name(process_state_t state);
uint32_t process_current_pid(void);
size_t process_get_count(void);

#endif /* ACHARYAOS_PROCESS_H */
