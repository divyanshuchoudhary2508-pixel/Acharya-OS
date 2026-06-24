/*
 * AcharyaOS - service.c
 * ---------------------
 * Lightweight startup service registry.
 */

#include "service.h"
#include "kstring.h"
#include "timer.h"
#include "log.h"

static service_entry_t services[SERVICE_ENTRY_MAX];

static void service_copy_text(char *dest, size_t size, const char *src) {
    size_t i = 0;
    if (size == 0) {
        return;
    }
    while (i + 1 < size && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int service_find(const char *name) {
    for (size_t i = 0; i < SERVICE_ENTRY_MAX; i++) {
        if (services[i].name[0] != '\0' && strcmp(services[i].name, name) == 0) {
            return (int) i;
        }
    }
    return -1;
}

static void service_register(const char *name, const char *description, uint8_t enabled) {
    for (size_t i = 0; i < SERVICE_ENTRY_MAX; i++) {
        if (services[i].name[0] != '\0') {
            continue;
        }
        service_copy_text(services[i].name, sizeof(services[i].name), name);
        service_copy_text(services[i].description, sizeof(services[i].description), description);
        services[i].enabled = enabled;
        services[i].state = SERVICE_STOPPED;
        services[i].last_started_tick = 0;
        services[i].last_stopped_tick = 0;
        services[i].start_count = 0;
        services[i].stop_count = 0;
        return;
    }
}

static int service_run_hook(const char *name) {
    if (strcmp(name, "logger") == 0) {
        log_write(LOG_INFO, "service logger started");
    } else if (strcmp(name, "clock") == 0) {
        log_write(LOG_INFO, "service clock started");
    } else if (strcmp(name, "shell-ready") == 0) {
        log_write(LOG_INFO, "service shell-ready started");
    }
    return 0;
}

static int service_stop_hook(const char *name) {
    if (strcmp(name, "logger") == 0) {
        log_write(LOG_INFO, "service logger stopped");
    } else if (strcmp(name, "clock") == 0) {
        log_write(LOG_INFO, "service clock stopped");
    } else if (strcmp(name, "shell-ready") == 0) {
        log_write(LOG_INFO, "service shell-ready stopped");
    }
    return 0;
}

const char *service_state_name(service_state_t state) {
    switch (state) {
        case SERVICE_STOPPED: return "stopped";
        case SERVICE_RUNNING: return "running";
        case SERVICE_FAILED:  return "failed";
        default: return "unknown";
    }
}

void service_init(void) {
    memset(services, 0, sizeof(services));
    service_register("logger", "kernel logging hook", 1);
    service_register("clock", "timer bookkeeping hook", 1);
    service_register("shell-ready", "shell readiness marker", 1);
}

static int service_mark_running(int idx) {
    services[idx].state = SERVICE_RUNNING;
    services[idx].last_started_tick = timer_get_ticks();
    services[idx].start_count++;
    return 0;
}

static int service_mark_stopped(int idx) {
    services[idx].state = SERVICE_STOPPED;
    services[idx].last_stopped_tick = timer_get_ticks();
    services[idx].stop_count++;
    return 0;
}

int service_start(const char *name) {
    int idx = service_find(name);
    if (idx < 0) {
        return -1;
    }
    if (service_run_hook(services[idx].name) != 0) {
        services[idx].state = SERVICE_FAILED;
        return -1;
    }
    return service_mark_running(idx);
}

int service_stop(const char *name) {
    int idx = service_find(name);
    if (idx < 0) {
        return -1;
    }
    if (services[idx].state != SERVICE_RUNNING) {
        return 0;
    }
    if (service_stop_hook(services[idx].name) != 0) {
        services[idx].state = SERVICE_FAILED;
        return -1;
    }
    return service_mark_stopped(idx);
}

int service_restart(const char *name) {
    if (service_stop(name) != 0) {
        return -1;
    }
    return service_start(name);
}

int service_enable(const char *name) {
    int idx = service_find(name);
    if (idx < 0) {
        return -1;
    }
    services[idx].enabled = 1;
    return 0;
}

int service_disable(const char *name) {
    int idx = service_find(name);
    if (idx < 0) {
        return -1;
    }
    services[idx].enabled = 0;
    return 0;
}

void service_startup_all(void) {
    for (size_t i = 0; i < SERVICE_ENTRY_MAX; i++) {
        if (services[i].name[0] == '\0' || !services[i].enabled) {
            continue;
        }
        (void) service_start(services[i].name);
    }
}

size_t service_list(service_entry_t *out_entries, size_t max_entries) {
    size_t count = 0;
    if (!out_entries || max_entries == 0) {
        return 0;
    }
    for (size_t i = 0; i < SERVICE_ENTRY_MAX && count < max_entries; i++) {
        if (services[i].name[0] == '\0') {
            continue;
        }
        out_entries[count++] = services[i];
    }
    return count;
}

int service_info(const char *name, service_entry_t *out_entry) {
    int idx;
    if (!name || !out_entry) {
        return -1;
    }
    idx = service_find(name);
    if (idx < 0) {
        return -1;
    }
    *out_entry = services[idx];
    return 0;
}

void service_get_stats(service_stats_t *stats) {
    if (!stats) {
        return;
    }
    stats->total_services = 0;
    stats->enabled_services = 0;
    stats->running_services = 0;
    for (size_t i = 0; i < SERVICE_ENTRY_MAX; i++) {
        if (services[i].name[0] == '\0') {
            continue;
        }
        stats->total_services++;
        if (services[i].enabled) {
            stats->enabled_services++;
        }
        if (services[i].state == SERVICE_RUNNING) {
            stats->running_services++;
        }
    }
}
