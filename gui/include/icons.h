/*
 * AcharyaOS - icons.h
 * -------------------
 * Phase 3, Feature 32: icon subsystem.
 *
 * Desktop icons are small launchable entries rendered over the framebuffer.
 * They are intentionally simple: a registry, a renderer, and click state.
 */

#ifndef ACHARYAOS_ICONS_H
#define ACHARYAOS_ICONS_H

#include <stddef.h>
#include <stdint.h>

#define ICONS_MAX_ICONS 16
#define ICONS_LABEL_MAX  24

typedef struct {
    int used;
    int selected;
    uint32_t id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t color;
    char label[ICONS_LABEL_MAX];
    char launch_cmd[ICONS_LABEL_MAX];
} icon_t;

typedef struct {
    size_t icon_count;
    uint32_t next_icon_id;
    uint32_t selected_icon_id;
} icon_stats_t;

void icons_init(void);
int icons_ready(void);
uint32_t icons_add(const char *label, const char *launch_cmd, uint32_t color);
void icons_add_default_icons(void);
void icons_render(void);
int icons_handle_click(int x, int y, uint8_t button_mask);
void icons_get_stats(icon_stats_t *stats);
size_t icons_copy(icon_t *out, size_t max_icons);
const icon_t *icons_get(uint32_t id);
int icons_launch(uint32_t id);
int icons_selected_launch_command(char *out, size_t out_size);

#endif /* ACHARYAOS_ICONS_H */
