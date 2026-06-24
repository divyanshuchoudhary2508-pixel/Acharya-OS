/*
 * AcharyaOS - config.h
 * --------------------
 * Phase 1, Feature 19: Configuration System.
 *
 * This subsystem stores a small set of named settings and persists them
 * to the filesystem as plain text.
 */

#ifndef ACHARYAOS_CONFIG_H
#define ACHARYAOS_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include "fs.h"

#define CONFIG_KEY_MAX   32
#define CONFIG_VALUE_MAX  96
#define CONFIG_ENTRY_MAX  16

typedef struct {
    char key[CONFIG_KEY_MAX];
    char value[CONFIG_VALUE_MAX];
    uint8_t used;
} config_entry_t;

typedef struct {
    size_t total_entries;
    size_t used_entries;
    char backing_file[FS_NAME_MAX];
} config_stats_t;

void config_init(void);
void config_reset_defaults(void);
int config_load(void);
int config_save(void);

int config_set(const char *key, const char *value);
int config_get(const char *key, char *out_value, size_t out_size);
size_t config_list(config_entry_t *out_entries, size_t max_entries);
void config_get_stats(config_stats_t *stats);
const char *config_backing_file(void);

#endif /* ACHARYAOS_CONFIG_H */
