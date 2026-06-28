/*
 * AcharyaOS - desktop.c
 * ---------------------
 * Phase 3, Feature 31: desktop environment.
 *
 * Minimal native desktop built on existing framebuffer/window/menu/button
 * primitives. The goal is to render a recognizable desktop surface and keep
 * the data model small enough to extend later with real icon launching and
 * input routing.
 */

#include "desktop.h"
#include "framebuffer.h"
#include "window.h"
#include "menu.h"
#include "button.h"
#include "icons.h"
#include "kstring.h"

#define DESKTOP_BG              0xFF1D3557u
#define DESKTOP_TASKBAR_BG      0xFF10151Fu
#define DESKTOP_TASKBAR_H       34u
#define DESKTOP_START_W         84u
#define DESKTOP_CLOCK_W         72u
#define DESKTOP_ICON_W          64u
#define DESKTOP_ICON_H          64u
#define DESKTOP_ICON_GAP        18u
#define DESKTOP_ICON_PAD        18u

static desktop_icon_t desktop_icons[DESKTOP_MAX_ICONS];
static desktop_stats_t desktop_state;
static int desktop_is_ready;
static uint32_t desktop_menu_id;
static uint32_t desktop_window_id;
static uint32_t desktop_start_button_id;

static void desktop_copy_text(char *dest, size_t size, const char *src) {
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

static desktop_icon_t *desktop_find_icon(uint32_t id) {
    for (size_t i = 0; i < DESKTOP_MAX_ICONS; i++) {
        if (desktop_icons[i].used && desktop_icons[i].id == id) {
            return &desktop_icons[i];
        }
    }
    return NULL;
}

static void desktop_clear_selection(void) {
    for (size_t i = 0; i < DESKTOP_MAX_ICONS; i++) {
        desktop_icons[i].selected = 0;
    }
    desktop_state.selected_icon_id = 0;
}

static void desktop_draw_icon(const desktop_icon_t *icon, uint32_t x, uint32_t y) {
    uint32_t frame_color = icon->selected ? 0xFFF4A261u : 0xFF0B1320u;
    fb_fill_rect(x - 2u, y - 2u, DESKTOP_ICON_W + 4u, DESKTOP_ICON_H + 24u, frame_color);
    fb_fill_rect(x, y, DESKTOP_ICON_W, DESKTOP_ICON_H, icon->color);
    fb_fill_rect(x + 18u, y + 18u, 28u, 28u, 0xFFE9ECEFu);
}

static void desktop_draw_taskbar(void) {
    uint32_t screen_w = 0;
    uint32_t screen_h = 0;
    fb_info_t info;

    fb_get_info(&info);
    screen_w = info.width;
    screen_h = info.height;
    if (screen_h < DESKTOP_TASKBAR_H || screen_w == 0) {
        return;
    }

    fb_fill_rect(0u, screen_h - DESKTOP_TASKBAR_H, screen_w, DESKTOP_TASKBAR_H, DESKTOP_TASKBAR_BG);
    fb_fill_rect(0u, screen_h - DESKTOP_TASKBAR_H, DESKTOP_START_W, DESKTOP_TASKBAR_H, 0xFF2A9D8Fu);
    fb_fill_rect(screen_w - DESKTOP_CLOCK_W, screen_h - DESKTOP_TASKBAR_H, DESKTOP_CLOCK_W, DESKTOP_TASKBAR_H, 0xFF243447u);

    if (desktop_start_button_id != 0) {
        btn_set_pressed(desktop_start_button_id, 0);
    }
}

void desktop_init(void) {
    fb_info_t info;
    for (size_t i = 0; i < DESKTOP_MAX_ICONS; i++) {
        memset(&desktop_icons[i], 0, sizeof(desktop_icons[i]));
    }
    fb_get_info(&info);
    desktop_state.taskbar_height = DESKTOP_TASKBAR_H;
    desktop_is_ready = fb_ready() && wm_ready() && menu_ready();
    desktop_window_id = 0;
    desktop_menu_id = 0;
    desktop_start_button_id = 0;

    icons_init();
    if (desktop_is_ready) {
        desktop_window_id = wm_create_window("desktop", 8u, 8u,
                                             info.width > 16u ? info.width - 16u : info.width,
                                             info.height > 16u ? info.height - 16u : info.height);
        desktop_menu_id = menu_create(desktop_window_id, "Desktop", 8u, 8u, 120u);
        if (desktop_menu_id != 0) {
            menu_add_item(desktop_menu_id, "Terminal", 1u);
            menu_add_item(desktop_menu_id, "Files", 2u);
            menu_add_item(desktop_menu_id, "Settings", 3u);
        }
        desktop_start_button_id = btn_create(desktop_window_id, "Start", 4u, info.height - DESKTOP_TASKBAR_H + 4u, 72u, 24u);
    }
}

int desktop_ready(void) {
    return desktop_is_ready && fb_ready();
}

uint32_t desktop_add_icon(const char *label, const char *launch_cmd, uint32_t color) {
    return icons_add(label, launch_cmd, color);
}

void desktop_add_default_icons(void) {
    icons_add_default_icons();
}

void desktop_render_taskbar(void) {
    if (!desktop_ready()) {
        return;
    }
    desktop_draw_taskbar();
}

void desktop_render(void) {
    fb_info_t info;
    fb_get_info(&info);
    if (!desktop_ready()) {
        return;
    }
    fb_fill_rect(0u, 0u, info.width, info.height, DESKTOP_BG);
    wm_render();
    icons_render();
    menu_render();
    desktop_draw_taskbar();
}

void desktop_handle_click(int x, int y, uint8_t button_mask) {
    int handled = icons_handle_click(x, y, button_mask);
    if (handled > 0) {
        icon_stats_t stats;
        icons_get_stats(&stats);
        desktop_state.icon_count = stats.icon_count;
        desktop_state.selected_icon_id = stats.selected_icon_id;
        desktop_state.next_icon_id = stats.next_icon_id;
        return;
    }
    desktop_clear_selection();
}

void desktop_handle_mouse_move(int x, int y) {
    (void)x;
    (void)y;
}

void desktop_update_clock(void) {
    desktop_draw_taskbar();
}

void desktop_get_stats(desktop_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = desktop_state;
    {
        icon_stats_t icon_stats;
        icons_get_stats(&icon_stats);
        stats->icon_count = icon_stats.icon_count;
        stats->selected_icon_id = icon_stats.selected_icon_id;
        stats->next_icon_id = icon_stats.next_icon_id;
    }
}

size_t desktop_copy_icons(desktop_icon_t *out, size_t max_icons) {
    return 0;
}
