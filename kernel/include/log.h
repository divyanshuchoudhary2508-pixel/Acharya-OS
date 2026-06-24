/*
 * AcharyaOS - log.h
 * ------------------
 * Phase 1, Feature 15: Logging and Debugging Framework.
 *
 * A kernel logger is not the same thing as printf. printf writes text to
 * the console. A logger records structured events so we can inspect what
 * happened later, even if the screen scrolls away or a subsystem fails
 * before it finishes booting.
 */

#ifndef ACHARYAOS_LOG_H
#define ACHARYAOS_LOG_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG = 1,
    LOG_INFO  = 2,
    LOG_WARN  = 3,
    LOG_ERROR = 4,
    LOG_PANIC = 5
} log_level_t;

#define LOG_BUFFER_MAX       64
#define LOG_MESSAGE_MAX      128

typedef struct {
    uint64_t tick;
    log_level_t level;
    char message[LOG_MESSAGE_MAX];
} log_entry_t;

typedef struct {
    uint32_t total_entries;
    uint32_t stored_entries;
    uint32_t dropped_entries;
    log_level_t minimum_level;
} log_stats_t;

void log_init(void);
void log_set_minimum_level(log_level_t level);
log_level_t log_get_minimum_level(void);

void log_write(log_level_t level, const char *message);
void log_writef(log_level_t level, const char *fmt, ...);

size_t log_copy_entries(log_entry_t *out_entries, size_t max_entries);
void log_get_stats(log_stats_t *stats);
const char *log_level_name(log_level_t level);

#endif /* ACHARYAOS_LOG_H */
