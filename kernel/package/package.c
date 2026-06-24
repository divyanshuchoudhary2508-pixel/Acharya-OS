/*
 * AcharyaOS - package.c
 * ---------------------
 * Text-manifest package installer.
 */

#include "package.h"
#include "fs.h"
#include "kstring.h"

#define PACKAGE_INDEX_FILE "packages.db"

static package_entry_t package_entries[PACKAGE_ENTRY_MAX];

static void package_clear_entries(void) {
    memset(package_entries, 0, sizeof(package_entries));
}

static void package_copy_text(char *dest, size_t size, const char *src) {
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

static int package_find_index(const char *name) {
    for (size_t i = 0; i < PACKAGE_ENTRY_MAX; i++) {
        if (package_entries[i].used && strcmp(package_entries[i].name, name) == 0) {
            return (int) i;
        }
    }
    return -1;
}

static int package_find_free_index(void) {
    for (size_t i = 0; i < PACKAGE_ENTRY_MAX; i++) {
        if (!package_entries[i].used) {
            return (int) i;
        }
    }
    return -1;
}

static const char *package_skip_spaces(const char *s) {
    while (*s == ' ') {
        s++;
    }
    return s;
}

static int package_starts_with_word(const char *line, const char *word) {
    size_t n = strlen(word);
    if (memcmp(line, word, n) != 0) {
        return 0;
    }
    return line[n] == '\0' || line[n] == ' ' || line[n] == '=';
}

static size_t package_parse_size(const char *text) {
    size_t value = 0;
    while (*text >= '0' && *text <= '9') {
        value = (value * 10) + (size_t) (*text - '0');
        text++;
    }
    return value;
}

static void package_trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

static void package_save_index(void) {
    char buffer[PACKAGE_MANIFEST_MAX];
    size_t pos = 0;

    for (size_t i = 0; i < PACKAGE_ENTRY_MAX; i++) {
        if (!package_entries[i].used) {
            continue;
        }
        for (size_t j = 0; package_entries[i].name[j] != '\0' && pos + 1 < sizeof(buffer); j++) {
            buffer[pos++] = package_entries[i].name[j];
        }
        if (pos + 1 >= sizeof(buffer)) {
            break;
        }
        buffer[pos++] = '|';
        for (size_t j = 0; package_entries[i].version[j] != '\0' && pos + 1 < sizeof(buffer); j++) {
            buffer[pos++] = package_entries[i].version[j];
        }
        if (pos + 1 >= sizeof(buffer)) {
            break;
        }
        buffer[pos++] = '|';
        for (size_t j = 0; package_entries[i].source[j] != '\0' && pos + 1 < sizeof(buffer); j++) {
            buffer[pos++] = package_entries[i].source[j];
        }
        if (pos + 1 >= sizeof(buffer)) {
            break;
        }
        buffer[pos++] = '|';
        {
            char digits[16];
            size_t n = 0;
            size_t count = package_entries[i].installed_files;
            if (count == 0) {
                digits[n++] = '0';
            } else {
                while (count > 0 && n < sizeof(digits)) {
                    digits[n++] = (char) ('0' + (count % 10));
                    count /= 10;
                }
            }
            while (n > 0 && pos + 1 < sizeof(buffer)) {
                buffer[pos++] = digits[--n];
            }
        }
        if (pos + 1 >= sizeof(buffer)) {
            break;
        }
        buffer[pos++] = '\n';
    }

    if (pos == 0) {
        buffer[pos++] = '\n';
    }

    (void) fs_write_file(PACKAGE_INDEX_FILE, buffer, pos);
}

static void package_load_index(void) {
    uint8_t buffer[PACKAGE_MANIFEST_MAX];
    size_t size = 0;
    char *text;

    package_clear_entries();
    if (fs_read_file(PACKAGE_INDEX_FILE, buffer, sizeof(buffer), &size) != 0) {
        return;
    }

    if (size >= sizeof(buffer)) {
        size = sizeof(buffer) - 1;
    }
    buffer[size] = '\0';
    text = (char *) buffer;

    while (*text != '\0') {
        char *line = text;
        char *name;
        char *version;
        char *source;
        char *count;
        int idx;

        while (*text != '\0' && *text != '\n') {
            text++;
        }
        if (*text == '\n') {
            *text++ = '\0';
        }
        package_trim_newline(line);
        if (line[0] == '\0') {
            continue;
        }

        name = line;
        version = name;
        while (*version && *version != '|') {
            version++;
        }
        if (*version != '|') {
            continue;
        }
        *version++ = '\0';
        source = version;
        while (*source && *source != '|') {
            source++;
        }
        if (*source != '|') {
            continue;
        }
        *source++ = '\0';
        count = source;
        while (*count && *count != '|') {
            count++;
        }
        if (*count == '|') {
            *count = '\0';
        }

        idx = package_find_free_index();
        if (idx < 0) {
            break;
        }
        package_entries[idx].used = 1;
        package_copy_text(package_entries[idx].name, sizeof(package_entries[idx].name), name);
        package_copy_text(package_entries[idx].version, sizeof(package_entries[idx].version), version);
        package_copy_text(package_entries[idx].source, sizeof(package_entries[idx].source), source);
        package_entries[idx].installed_files = package_parse_size(count);
    }
}

void package_init(void) {
    package_load_index();
}

void package_get_stats(package_stats_t *stats) {
    if (!stats) {
        return;
    }
    stats->total_entries = PACKAGE_ENTRY_MAX;
    stats->used_entries = 0;
    for (size_t i = 0; i < PACKAGE_ENTRY_MAX; i++) {
        if (package_entries[i].used) {
            stats->used_entries++;
        }
    }
}

size_t package_list(package_entry_t *out_entries, size_t max_entries) {
    size_t count = 0;
    if (!out_entries || max_entries == 0) {
        return 0;
    }
    for (size_t i = 0; i < PACKAGE_ENTRY_MAX && count < max_entries; i++) {
        if (!package_entries[i].used) {
            continue;
        }
        out_entries[count++] = package_entries[i];
    }
    return count;
}

int package_info(const char *package_name, package_entry_t *out_entry) {
    int idx;
    if (!package_name || !out_entry) {
        return -1;
    }
    idx = package_find_index(package_name);
    if (idx < 0) {
        return -1;
    }
    *out_entry = package_entries[idx];
    return 0;
}

static int package_parse_manifest(const char *manifest, char package_name[PACKAGE_NAME_MAX],
                                 char package_version[PACKAGE_VERSION_MAX]) {
    char line[PACKAGE_MANIFEST_MAX];
    const char *text = manifest;
    int saw_header = 0;

    while (*text != '\0') {
        size_t i = 0;
        while (*text == '\r' || *text == '\n') {
            text++;
        }
        while (*text != '\0' && *text != '\n' && i + 1 < sizeof(line)) {
            if (*text != '\r') {
                line[i++] = *text;
            }
            text++;
        }
        line[i] = '\0';
        if (*text == '\n') {
            text++;
        }

        if (line[0] == '\0') {
            continue;
        }
        if (strcmp(line, "PKG1") == 0) {
            saw_header = 1;
            continue;
        }
        if (package_starts_with_word(line, "package")) {
            const char *value = package_skip_spaces(line + 7);
            if (*value == '=') {
                value++;
            }
            package_copy_text(package_name, PACKAGE_NAME_MAX, value);
        } else if (package_starts_with_word(line, "version")) {
            const char *value = package_skip_spaces(line + 7);
            if (*value == '=') {
                value++;
            }
            package_copy_text(package_version, PACKAGE_VERSION_MAX, value);
        }
    }

    return saw_header && package_name[0] != '\0';
}

int package_install(const char *manifest_name) {
    uint8_t buffer[PACKAGE_MANIFEST_MAX];
    size_t size = 0;
    char *text;
    char package_name[PACKAGE_NAME_MAX];
    char package_version[PACKAGE_VERSION_MAX];
    int idx;
    size_t installed_files = 0;

    if (!manifest_name || *manifest_name == '\0') {
        return -1;
    }

    if (fs_read_file(manifest_name, buffer, sizeof(buffer), &size) != 0) {
        return -1;
    }
    if (size >= sizeof(buffer)) {
        size = sizeof(buffer) - 1;
    }
    buffer[size] = '\0';

    package_name[0] = '\0';
    package_version[0] = '\0';
    if (!package_parse_manifest((const char *) buffer, package_name, package_version)) {
        return -1;
    }

    text = (char *) buffer;
    while (*text != '\0') {
        char *line = text;
        while (*text != '\0' && *text != '\n') {
            text++;
        }
        if (*text == '\n') {
            *text++ = '\0';
        }
        package_trim_newline(line);
        if (package_starts_with_word(line, "file")) {
            char *payload = line + 4;
            char *sep;
            while (*payload == ' ' || *payload == '=') {
                payload++;
            }
            sep = payload;
            while (*sep && *sep != '|') {
                sep++;
            }
            if (*sep != '|') {
                return -1;
            }
            *sep++ = '\0';
            if (fs_write_file(payload, sep, strlen(sep)) != 0) {
                return -1;
            }
            installed_files++;
        }
        if (strcmp(line, "end") == 0) {
            break;
        }
    }

    idx = package_find_index(package_name);
    if (idx < 0) {
        idx = package_find_free_index();
        if (idx < 0) {
            return -1;
        }
        package_entries[idx].used = 1;
    }
    package_copy_text(package_entries[idx].name, sizeof(package_entries[idx].name), package_name);
    package_copy_text(package_entries[idx].version, sizeof(package_entries[idx].version), package_version);
    package_copy_text(package_entries[idx].source, sizeof(package_entries[idx].source), manifest_name);
    package_entries[idx].installed_files = installed_files;

    package_save_index();
    return 0;
}

int package_remove(const char *package_name) {
    int idx = package_find_index(package_name);
    if (idx < 0) {
        return -1;
    }
    memset(&package_entries[idx], 0, sizeof(package_entries[idx]));
    package_save_index();
    return 0;
}
