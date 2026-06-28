/*
 * AcharyaOS - settings_panels.c
 * -----------------------------
 * Feature 34 panel assembly helpers.
 */

#include "settings_panels.h"
#include "settings_data.h"

void settings_panels_build(settings_panel_t *out, size_t max_panels) {
    (void) settings_data_copy_panels(out, max_panels);
}

