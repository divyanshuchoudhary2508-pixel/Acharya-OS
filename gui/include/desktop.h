/*
 * AcharyaOS - desktop.h
 * ---------------------
 * Phase 3, Feature 31: desktop environment.
 *
 * This is a minimal AcharyaOS-native desktop layer built on top of the
 * framebuffer, window manager, menu, and button primitives that already
 * exist in the tree. It does not depend on the foreign drop-in code layout.
 */

#ifndef ACHARYAOS_DESKTOP_H
#define ACHARYAOS_DESKTOP_H

#include <stddef.h>
#include <stdint.h>

#define DESKTOP_MAX_ICONS  8
#define DESKTOP_LABEL_MAX   24

typedef struct {
    uint8_t used;
    uint8_t selected;
    uint32_t id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t color;
    char label[DESKTOP_LABEL_MAX];
    char launch_cmd[DESKTOP_LABEL_MAX];
} desktop_icon_t;

typedef struct {
    size_t icon_count;
    uint32_t selected_icon_id;
    uint32_t next_icon_id;
    uint32_t taskbar_height;
} desktop_stats_t;

void desktop_init(void);
int desktop_ready(void);
uint32_t desktop_add_icon(const char *label, const char *launch_cmd, uint32_t color);
void desktop_add_default_icons(void);
void desktop_render(void);
void desktop_render_taskbar(void);
void desktop_handle_click(int x, int y, uint8_t button_mask);
void desktop_handle_mouse_move(int x, int y);
void desktop_update_clock(void);
void desktop_get_stats(desktop_stats_t *stats);
size_t desktop_copy_icons(desktop_icon_t *out, size_t max_icons);

#endif /* ACHARYAOS_DESKTOP_H */
