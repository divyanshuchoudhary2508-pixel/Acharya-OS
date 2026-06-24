/*
 * AcharyaOS - service.h
 * ---------------------
 * Phase 1, Feature 21: Startup Services.
 */

#ifndef ACHARYAOS_SERVICE_H
#define ACHARYAOS_SERVICE_H

#include <stddef.h>
#include <stdint.h>

#define SERVICE_NAME_MAX   32
#define SERVICE_DESC_MAX   64
#define SERVICE_ENTRY_MAX  16

typedef enum {
    SERVICE_STOPPED = 0,
    SERVICE_RUNNING = 1,
    SERVICE_FAILED  = 2
} service_state_t;

typedef struct {
    char name[SERVICE_NAME_MAX];
    char description[SERVICE_DESC_MAX];
    uint8_t enabled;
    service_state_t state;
    uint64_t last_started_tick;
    uint64_t last_stopped_tick;
    uint32_t start_count;
    uint32_t stop_count;
} service_entry_t;

typedef struct {
    size_t total_services;
    size_t enabled_services;
    size_t running_services;
} service_stats_t;

void service_init(void);
void service_startup_all(void);
size_t service_list(service_entry_t *out_entries, size_t max_entries);
int service_info(const char *name, service_entry_t *out_entry);
int service_start(const char *name);
int service_stop(const char *name);
int service_restart(const char *name);
int service_enable(const char *name);
int service_disable(const char *name);
void service_get_stats(service_stats_t *stats);
const char *service_state_name(service_state_t state);

#endif /* ACHARYAOS_SERVICE_H */
