/*
 * AcharyaOS - settings_data.c
 * ---------------------------
 * Feature 34 settings storage.
 */

#include "settings_data.h"
#include "config.h"
#include "kstring.h"

static settings_panel_t g_panels[SETTINGS_MAX_PANELS];
static settings_panel_id_t g_active_panel = SETTINGS_PANEL_DISPLAY;

static void copy_text(char *dest, size_t size, const char *src) {
    size_t i = 0;
    if (size == 0) {
        return;
    }
    if (!src) {
        dest[0] = '\0';
        return;
    }
    while (i + 1 < size && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void settings_data_init(void) {
    memset(g_panels, 0, sizeof(g_panels));
    g_active_panel = SETTINGS_PANEL_DISPLAY;
    settings_data_apply_defaults();
}

void settings_data_apply_defaults(void) {
    settings_panel_t *p;

    memset(g_panels, 0, sizeof(g_panels));

    p = &g_panels[SETTINGS_PANEL_DISPLAY];
    p->id = SETTINGS_PANEL_DISPLAY;
    p->title = "Display";
    p->item_count = 3;
    p->items[0].key = "prompt";
    p->items[1].key = "editor_tabwidth";
    p->items[2].key = "loglevel";

    p = &g_panels[SETTINGS_PANEL_AUDIO];
    p->id = SETTINGS_PANEL_AUDIO;
    p->title = "Audio";
    p->item_count = 2;
    p->items[0].key = "audio_volume";
    p->items[1].key = "audio_mute";

    p = &g_panels[SETTINGS_PANEL_NETWORK];
    p->id = SETTINGS_PANEL_NETWORK;
    p->title = "Network";
    p->item_count = 2;
    p->items[0].key = "hostname";
    p->items[1].key = "dns_server";

    p = &g_panels[SETTINGS_PANEL_USERS];
    p->id = SETTINGS_PANEL_USERS;
    p->title = "Users";
    p->item_count = 2;
    p->items[0].key = "default_user";
    p->items[1].key = "guest_login";

    p = &g_panels[SETTINGS_PANEL_ABOUT];
    p->id = SETTINGS_PANEL_ABOUT;
    p->title = "About";
    p->item_count = 2;
    p->items[0].key = "os_name";
    p->items[1].key = "os_version";

    settings_data_refresh_from_config();
}

void settings_data_refresh_from_config(void) {
    char value[96];
    size_t i;

    for (i = 0; i < SETTINGS_MAX_PANELS; i++) {
        settings_panel_t *p = &g_panels[i];
        for (size_t j = 0; j < p->item_count; j++) {
            if (config_get(p->items[j].key, value, sizeof(value)) == 0) {
                copy_text(p->items[j].value, sizeof(p->items[j].value), value);
            } else {
                p->items[j].value[0] = '\0';
            }
        }
    }
}

size_t settings_data_copy_panels(settings_panel_t *out, size_t max_panels) {
    size_t count = 0;
    if (!out) {
        return 0;
    }
    for (size_t i = 0; i < SETTINGS_MAX_PANELS && count < max_panels; i++) {
        out[count++] = g_panels[i];
    }
    return count;
}

settings_panel_t *settings_data_active_panel(void) {
    return &g_panels[g_active_panel];
}

void settings_data_set_active(settings_panel_id_t panel) {
    if (panel < SETTINGS_MAX_PANELS) {
        g_active_panel = panel;
    }
}

settings_panel_id_t settings_data_active(void) {
    return g_active_panel;
}
