/*
 * AcharyaOS - settings.h
 * ----------------------
 * Phase 3, Feature 34: settings application.
 *
 * The settings app is a small panel-based GUI layer that reads from the
 * kernel configuration store and renders a handful of categorized views.
 */

#ifndef ACHARYAOS_SETTINGS_H
#define ACHARYAOS_SETTINGS_H

#include <stddef.h>
#include <stdint.h>

#define SETTINGS_MAX_PANELS  5
#define SETTINGS_PANEL_TITLE_MAX 24

typedef enum {
    SETTINGS_PANEL_DISPLAY = 0,
    SETTINGS_PANEL_AUDIO,
    SETTINGS_PANEL_NETWORK,
    SETTINGS_PANEL_USERS,
    SETTINGS_PANEL_ABOUT
} settings_panel_id_t;

typedef struct {
    const char *key;
    char value[96];
} settings_item_t;

typedef struct {
    settings_panel_id_t id;
    const char *title;
    size_t item_count;
    settings_item_t items[8];
} settings_panel_t;

typedef struct {
    int ready;
    settings_panel_id_t active_panel;
    size_t panel_count;
    uint32_t render_count;
} settings_stats_t;

void settings_init(void);
int settings_ready(void);
void settings_render(void);
void settings_show_panel(settings_panel_id_t panel);
void settings_get_stats(settings_stats_t *stats);
size_t settings_copy_panels(settings_panel_t *out, size_t max_panels);
void settings_refresh_from_config(void);
void settings_apply_defaults(void);

#endif /* ACHARYAOS_SETTINGS_H */
