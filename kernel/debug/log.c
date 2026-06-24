/*
 * AcharyaOS - log.c
 * -----------------
 * Small ring-buffer logger for kernel diagnostics.
 */

#include "log.h"
#include "kio.h"
#include "kstring.h"
#include "timer.h"
#include "kernel.h"
#include <stdarg.h>

static log_entry_t log_buffer[LOG_BUFFER_MAX];
static uint32_t log_head;
static uint32_t log_count;
static uint32_t log_total;
static uint32_t log_dropped;
static log_level_t log_minimum_level = LOG_INFO;

static char log_level_letter(log_level_t level) {
    switch (level) {
        case LOG_TRACE: return 'T';
        case LOG_DEBUG: return 'D';
        case LOG_INFO:  return 'I';
        case LOG_WARN:  return 'W';
        case LOG_ERROR: return 'E';
        case LOG_PANIC: return 'P';
        default:        return '?';
    }
}

const char *log_level_name(log_level_t level) {
    switch (level) {
        case LOG_TRACE: return "trace";
        case LOG_DEBUG: return "debug";
        case LOG_INFO:  return "info";
        case LOG_WARN:  return "warn";
        case LOG_ERROR: return "error";
        case LOG_PANIC: return "panic";
        default:        return "unknown";
    }
}

void log_init(void) {
    log_head = 0;
    log_count = 0;
    log_total = 0;
    log_dropped = 0;
    log_minimum_level = LOG_INFO;
}

void log_set_minimum_level(log_level_t level) {
    log_minimum_level = level;
}

log_level_t log_get_minimum_level(void) {
    return log_minimum_level;
}

static void log_store_entry(log_level_t level, const char *message) {
    log_entry_t *entry;
    size_t i = 0;

    if (log_count < LOG_BUFFER_MAX) {
        entry = &log_buffer[log_head];
        log_count++;
    } else {
        entry = &log_buffer[log_head];
        log_dropped++;
    }

    entry->tick = timer_get_ticks();
    entry->level = level;
    while (i < LOG_MESSAGE_MAX - 1 && message[i] != '\0') {
        entry->message[i] = message[i];
        i++;
    }
    entry->message[i] = '\0';

    log_head = (log_head + 1) % LOG_BUFFER_MAX;
    log_total++;
}

static void log_append_char(char *buffer, size_t size, size_t *pos, char c) {
    if (*pos + 1 < size) {
        buffer[*pos] = c;
    }
    (*pos)++;
}

static void log_append_str(char *buffer, size_t size, size_t *pos, const char *s) {
    while (*s) {
        log_append_char(buffer, size, pos, *s);
        s++;
    }
}

static void log_append_uint(char *buffer, size_t size, size_t *pos,
                            uint64_t value, uint32_t base, int uppercase) {
    char tmp[32];
    size_t len = 0;

    if (value == 0) {
        log_append_char(buffer, size, pos, '0');
        return;
    }

    while (value > 0 && len < sizeof(tmp)) {
        uint64_t digit = value % base;
        tmp[len++] = (char) ((digit < 10)
                             ? ('0' + digit)
                             : ((uppercase ? 'A' : 'a') + (digit - 10)));
        value /= base;
    }

    while (len > 0) {
        log_append_char(buffer, size, pos, tmp[--len]);
    }
}

static void log_append_int(char *buffer, size_t size, size_t *pos, int64_t value) {
    if (value < 0) {
        uint64_t magnitude;
        log_append_char(buffer, size, pos, '-');
        magnitude = (uint64_t)(-(value + 1)) + 1;
        log_append_uint(buffer, size, pos, magnitude, 10, 0);
    } else {
        log_append_uint(buffer, size, pos, (uint64_t) value, 10, 0);
    }
}

static void log_format_message(char *buffer, size_t size, const char *fmt, va_list args) {
    size_t pos = 0;

    if (size == 0) {
        return;
    }

    while (*fmt) {
        if (*fmt != '%') {
            log_append_char(buffer, size, &pos, *fmt++);
            continue;
        }

        fmt++;
        switch (*fmt) {
            case 's':
                log_append_str(buffer, size, &pos, va_arg(args, const char *));
                break;
            case 'd':
                log_append_int(buffer, size, &pos, (int64_t) va_arg(args, int32_t));
                break;
            case 'x':
                log_append_uint(buffer, size, &pos,
                                (uint64_t) va_arg(args, uint32_t),
                                16, 0);
                break;
            case 'c':
                log_append_char(buffer, size, &pos, (char) va_arg(args, int));
                break;
            case '%':
                log_append_char(buffer, size, &pos, '%');
                break;
            case '\0':
                log_append_char(buffer, size, &pos, '%');
                goto done;
            default:
                log_append_char(buffer, size, &pos, '%');
                log_append_char(buffer, size, &pos, *fmt);
                break;
        }
        fmt++;
    }

done:
    if (pos >= size) {
        buffer[size - 1] = '\0';
    } else {
        buffer[pos] = '\0';
    }
}

void log_write(log_level_t level, const char *message) {
    if (level < log_minimum_level) {
        return;
    }

    log_store_entry(level, message);

    kprintf("[%c][%d] %s\n",
            log_level_letter(level),
            (int32_t) timer_get_ticks(),
            message);
}

static void log_vwritef(log_level_t level, const char *fmt, va_list args) {
    char buffer[LOG_MESSAGE_MAX];

    buffer[0] = '\0';
    log_format_message(buffer, sizeof(buffer), fmt, args);
    log_write(level, buffer);
}

void log_writef(log_level_t level, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    log_vwritef(level, fmt, args);
    va_end(args);
}

size_t log_copy_entries(log_entry_t *out_entries, size_t max_entries) {
    size_t copied = 0;

    if (out_entries == NULL || max_entries == 0) {
        return 0;
    }

    if (log_count == 0) {
        return 0;
    }

    {
        size_t start = (log_head + LOG_BUFFER_MAX - log_count) % LOG_BUFFER_MAX;
        for (; copied < log_count && copied < max_entries; copied++) {
            out_entries[copied] = log_buffer[(start + copied) % LOG_BUFFER_MAX];
        }
    }

    return copied;
}

void log_get_stats(log_stats_t *stats) {
    if (stats == NULL) {
        return;
    }

    stats->total_entries = log_total;
    stats->stored_entries = log_count;
    stats->dropped_entries = log_dropped;
    stats->minimum_level = log_minimum_level;
}
