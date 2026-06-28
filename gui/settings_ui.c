/*
 * AcharyaOS - settings_ui.c
 * -------------------------
 * Feature 34 reusable settings widgets.
 */

#include "settings_ui.h"
#include "framebuffer.h"

#define SETTINGS_BG         0xFF112233u
#define SETTINGS_SIDEBAR    0xFF1E2A38u
#define SETTINGS_PANEL_BG   0xFF243447u
#define SETTINGS_BORDER     0xFF0B1320u
#define SETTINGS_ACCENT     0xFF4C78A8u
#define SETTINGS_HILITE     0xFFF4A261u
#define SETTINGS_TEXT_BOX   0xFFE9ECEFu

void settings_ui_draw_frame(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    fb_fill_rect(x, y, w, h, SETTINGS_BORDER);
    if (w > 2u && h > 2u) {
        fb_fill_rect(x + 1u, y + 1u, w - 2u, h - 2u, SETTINGS_BG);
    }
}

void settings_ui_draw_sidebar(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    fb_fill_rect(x, y, w, h, SETTINGS_SIDEBAR);
    fb_fill_rect(x + 6u, y + 8u, w > 12u ? w - 12u : w, 18u, SETTINGS_ACCENT);
    fb_fill_rect(x + 6u, y + 34u, w > 12u ? w - 12u : w, 18u, SETTINGS_HILITE);
    fb_fill_rect(x + 6u, y + 60u, w > 12u ? w - 12u : w, 18u, SETTINGS_ACCENT);
    fb_fill_rect(x + 6u, y + 86u, w > 12u ? w - 12u : w, 18u, SETTINGS_HILITE);
    fb_fill_rect(x + 6u, y + 112u, w > 12u ? w - 12u : w, 18u, SETTINGS_ACCENT);
}

void settings_ui_draw_panel(const settings_panel_t *panel, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    uint32_t i;
    if (!panel) {
        return;
    }
    fb_fill_rect(x, y, w, h, SETTINGS_PANEL_BG);
    for (i = 0; i < panel->item_count; i++) {
        uint32_t row_y = y + 16u + (i * 24u);
        fb_fill_rect(x + 16u, row_y, w > 32u ? w - 32u : w, 16u, SETTINGS_TEXT_BOX);
        if (panel->items[i].value[0] != '\0') {
            fb_fill_rect(x + 20u, row_y + 4u, 8u, 8u, SETTINGS_ACCENT);
        }
    }
}
