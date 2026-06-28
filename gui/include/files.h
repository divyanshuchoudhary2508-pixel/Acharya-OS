/*
 * AcharyaOS - files.h
 * -------------------
 * Phase 3, Feature 35: graphical file explorer.
 */

#ifndef ACHARYAOS_FILES_H
#define ACHARYAOS_FILES_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int ready;
    uint32_t render_count;
    size_t file_count;
    size_t selected_index;
} files_stats_t;

void files_init(void);
int files_ready(void);
void files_render(void);
void files_get_stats(files_stats_t *stats);

#endif /* ACHARYAOS_FILES_H */

