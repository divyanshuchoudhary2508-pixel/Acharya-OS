/*
 * AcharyaOS - config.c
 * --------------------
 * Small text-backed configuration store.
 */

#include "config.h"
#include "kstring.h"

#define CONFIG_FILE_NAME "acharyaos.cfg"
#define CONFIG_FILE_MAX   1024

static config_entry_t config_entries[CONFIG_ENTRY_MAX];
static char config_file_name[FS_NAME_MAX] = CONFIG_FILE_NAME;

static const struct {
    const char *key;
    const char *value;
} config_defaults[] = {
    { "prompt", "acharyaos> " },
    { "loglevel", "info" },
    { "browser_filter", "" },
    { "editor_tabwidth", "4" },
};

static size_t config_default_count(void) {
    return sizeof(config_defaults) / sizeof(config_defaults[0]);
}

static void config_copy_text(char *dest, size_t dest_size, const char *src) {
    size_t i = 0;
    if (dest_size == 0) {
        return;
    }
    while (i + 1 < dest_size && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int config_find_index(const char *key) {
    for (size_t i = 0; i < CONFIG_ENTRY_MAX; i++) {
        if (config_entries[i].used && strcmp(config_entries[i].key, key) == 0) {
            return (int) i;
        }
    }
    return -1;
}

static int config_find_free_index(void) {
    for (size_t i = 0; i < CONFIG_ENTRY_MAX; i++) {
        if (!config_entries[i].used) {
            return (int) i;
        }
    }
    return -1;
}

static void config_clear(void) {
    memset(config_entries, 0, sizeof(config_entries));
}

void config_reset_defaults(void) {
    config_clear();
    for (size_t i = 0; i < config_default_count(); i++) {
        int idx = config_find_free_index();
        if (idx < 0) {
            break;
        }
        config_copy_text(config_entries[idx].key, sizeof(config_entries[idx].key), config_defaults[i].key);
        config_copy_text(config_entries[idx].value, sizeof(config_entries[idx].value), config_defaults[i].value);
        config_entries[idx].used = 1;
    }
}

void config_init(void) {
    config_copy_text(config_file_name, sizeof(config_file_name), CONFIG_FILE_NAME);
    config_reset_defaults();
    (void) config_load();
}

const char *config_backing_file(void) {
    return config_file_name;
}

int config_set(const char *key, const char *value) {
    int idx;

    if (!key || !value || *key == '\0') {
        return -1;
    }

    idx = config_find_index(key);
    if (idx < 0) {
        idx = config_find_free_index();
        if (idx < 0) {
            return -1;
        }
        config_entries[idx].used = 1;
        config_copy_text(config_entries[idx].key, sizeof(config_entries[idx].key), key);
    }

    config_copy_text(config_entries[idx].value, sizeof(config_entries[idx].value), value);
    return 0;
}

int config_get(const char *key, char *out_value, size_t out_size) {
    int idx;

    if (!key || !out_value || out_size == 0) {
        return -1;
    }

    idx = config_find_index(key);
    if (idx < 0) {
        return -1;
    }

    config_copy_text(out_value, out_size, config_entries[idx].value);
    return 0;
}

size_t config_list(config_entry_t *out_entries, size_t max_entries) {
    size_t count = 0;

    if (!out_entries || max_entries == 0) {
        return 0;
    }

    for (size_t i = 0; i < CONFIG_ENTRY_MAX && count < max_entries; i++) {
        if (!config_entries[i].used) {
            continue;
        }
        out_entries[count++] = config_entries[i];
    }
    return count;
}

void config_get_stats(config_stats_t *stats) {
    if (!stats) {
        return;
    }
    stats->total_entries = CONFIG_ENTRY_MAX;
    stats->used_entries = 0;
    for (size_t i = 0; i < CONFIG_ENTRY_MAX; i++) {
        if (config_entries[i].used) {
            stats->used_entries++;
        }
    }
    config_copy_text(stats->backing_file, sizeof(stats->backing_file), config_file_name);
}

static const char *config_next_line(const char *text, char *line, size_t line_size) {
    size_t i = 0;

    while (*text == '\r' || *text == '\n') {
        text++;
    }

    while (*text != '\0' && *text != '\n' && i + 1 < line_size) {
        if (*text != '\r') {
            line[i++] = *text;
        }
        text++;
    }

    line[i] = '\0';

    while (*text != '\0' && *text != '\n') {
        text++;
    }
    if (*text == '\n') {
        text++;
    }

    return text;
}

int config_load(void) {
    uint8_t buffer[CONFIG_FILE_MAX];
    size_t size = 0;
    const char *text;
    char line[CONFIG_KEY_MAX + CONFIG_VALUE_MAX + 2];

    config_reset_defaults();

    if (fs_read_file(config_file_name, buffer, sizeof(buffer), &size) != 0) {
        return 0;
    }

    if (size >= sizeof(buffer)) {
        size = sizeof(buffer) - 1;
    }
    buffer[size] = '\0';
    text = (const char *) buffer;

    while (*text != '\0') {
        char *eq;
        text = config_next_line(text, line, sizeof(line));
        if (line[0] == '\0') {
            continue;
        }
        eq = line;
        while (*eq != '\0' && *eq != '=') {
            eq++;
        }
        if (*eq != '=') {
            continue;
        }
        *eq = '\0';
        eq++;
        (void) config_set(line, eq);
    }

    return 0;
}

int config_save(void) {
    char buffer[CONFIG_FILE_MAX];
    size_t pos = 0;

    for (size_t i = 0; i < CONFIG_ENTRY_MAX; i++) {
        if (!config_entries[i].used) {
            continue;
        }

        for (size_t j = 0; config_entries[i].key[j] != '\0' && pos + 1 < sizeof(buffer); j++) {
            buffer[pos++] = config_entries[i].key[j];
        }
        if (pos + 1 >= sizeof(buffer)) {
            break;
        }
        buffer[pos++] = '=';
        for (size_t j = 0; config_entries[i].value[j] != '\0' && pos + 1 < sizeof(buffer); j++) {
            buffer[pos++] = config_entries[i].value[j];
        }
        if (pos + 1 >= sizeof(buffer)) {
            break;
        }
        buffer[pos++] = '\n';
    }

    if (pos == 0) {
        buffer[pos++] = '\n';
    }

    return fs_write_file(config_file_name, buffer, pos);
}
