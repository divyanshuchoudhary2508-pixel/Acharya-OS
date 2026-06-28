/*
 * AcharyaOS - settings_ui.h
 * -------------------------
 * Feature 34 reusable settings widget helpers.
 */

#ifndef ACHARYAOS_SETTINGS_UI_H
#define ACHARYAOS_SETTINGS_UI_H

#include <stdint.h>
#include "settings.h"

void settings_ui_draw_frame(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void settings_ui_draw_sidebar(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void settings_ui_draw_panel(const settings_panel_t *panel, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

#endif /* ACHARYAOS_SETTINGS_UI_H */
