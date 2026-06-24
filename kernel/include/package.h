/*
 * AcharyaOS - package.h
 * ---------------------
 * Phase 1, Feature 20: Package Installer.
 *
 * A package is a small text manifest that can describe files to unpack
 * into the simple filesystem. This is deliberately educational rather
 * than ambitious: it gives us installation semantics without requiring a
 * full repository, dependency solver, or directory tree.
 */

#ifndef ACHARYAOS_PACKAGE_H
#define ACHARYAOS_PACKAGE_H

#include <stddef.h>
#include <stdint.h>
#include "fs.h"

#define PACKAGE_NAME_MAX       32
#define PACKAGE_VERSION_MAX    16
#define PACKAGE_FILE_MAX       64
#define PACKAGE_ENTRY_MAX      12
#define PACKAGE_MANIFEST_MAX   2048

typedef struct {
    char name[PACKAGE_NAME_MAX];
    char version[PACKAGE_VERSION_MAX];
    char source[PACKAGE_FILE_MAX];
    size_t installed_files;
    uint8_t used;
} package_entry_t;

typedef struct {
    size_t total_entries;
    size_t used_entries;
} package_stats_t;

void package_init(void);
int package_install(const char *manifest_name);
int package_remove(const char *package_name);
size_t package_list(package_entry_t *out_entries, size_t max_entries);
int package_info(const char *package_name, package_entry_t *out_entry);
void package_get_stats(package_stats_t *stats);

#endif /* ACHARYAOS_PACKAGE_H */
