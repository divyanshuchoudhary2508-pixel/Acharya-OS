/*
 * AcharyaOS - settings_data.h
 * ---------------------------
 * Feature 34 settings data model.
 */

#ifndef ACHARYAOS_SETTINGS_DATA_H
#define ACHARYAOS_SETTINGS_DATA_H

#include "settings.h"

void settings_data_init(void);
void settings_data_apply_defaults(void);
void settings_data_refresh_from_config(void);
size_t settings_data_copy_panels(settings_panel_t *out, size_t max_panels);
settings_panel_t *settings_data_active_panel(void);
void settings_data_set_active(settings_panel_id_t panel);
settings_panel_id_t settings_data_active(void);

#endif /* ACHARYAOS_SETTINGS_DATA_H */
