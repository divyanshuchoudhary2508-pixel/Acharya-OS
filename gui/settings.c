/*
 * AcharyaOS - settings.c
 * ----------------------
 * Feature 34: settings application entry point.
 */

#include "settings.h"
#include "settings_data.h"
#include "settings_panels.h"
#include "settings_ui.h"
#include "framebuffer.h"

#define SETTINGS_VIEW_X      24u
#define SETTINGS_VIEW_Y      24u
#define SETTINGS_VIEW_W      720u
#define SETTINGS_VIEW_H      500u
#define SETTINGS_SIDEBAR_W   180u

static settings_stats_t g_stats;
static int g_ready = 0;
static settings_panel_t g_panels[SETTINGS_MAX_PANELS];

static void settings_refresh_stats(void) {
    g_stats.ready = g_ready;
    g_stats.active_panel = settings_data_active();
    g_stats.panel_count = settings_copy_panels(g_panels, SETTINGS_MAX_PANELS);
}

void settings_init(void) {
    g_ready = 1;
    g_stats.render_count = 0;
    settings_data_init();
    settings_refresh_from_config();
    settings_refresh_stats();
}

int settings_ready(void) {
    return g_ready;
}

void settings_show_panel(settings_panel_id_t panel) {
    settings_data_set_active(panel);
    settings_refresh_stats();
}

void settings_render(void) {
    settings_panel_t *panel;
    uint32_t sidebar_x = SETTINGS_VIEW_X + 1u;
    uint32_t sidebar_y = SETTINGS_VIEW_Y + 1u;
    uint32_t panel_x = SETTINGS_VIEW_X + SETTINGS_SIDEBAR_W + 1u;
    uint32_t panel_y = SETTINGS_VIEW_Y + 1u;
    uint32_t panel_w = SETTINGS_VIEW_W - SETTINGS_SIDEBAR_W - 2u;
    uint32_t panel_h = SETTINGS_VIEW_H - 2u;

    if (!g_ready || !fb_ready()) {
        return;
    }

    settings_ui_draw_frame(SETTINGS_VIEW_X, SETTINGS_VIEW_Y, SETTINGS_VIEW_W, SETTINGS_VIEW_H);
    settings_ui_draw_sidebar(sidebar_x, sidebar_y, SETTINGS_SIDEBAR_W, panel_h);

    panel = settings_data_active_panel();
    if (panel) {
        settings_ui_draw_panel(panel, panel_x, panel_y, panel_w, panel_h);
    }

    g_stats.render_count++;
    settings_refresh_stats();
}

void settings_get_stats(settings_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = g_stats;
}

size_t settings_copy_panels(settings_panel_t *out, size_t max_panels) {
    return settings_data_copy_panels(out, max_panels);
}

void settings_refresh_from_config(void) {
    settings_data_refresh_from_config();
    settings_refresh_stats();
}

void settings_apply_defaults(void) {
    settings_data_apply_defaults();
    settings_refresh_stats();
}

