/*
 * AcharyaOS - icons.c
 * -------------------
 * Phase 3, Feature 32: icon subsystem.
 */

#include "icons.h"
#include "framebuffer.h"
#include "window.h"
#include "kstring.h"
#include "timer.h"
#include "shell.h"

#define ICON_CELL_W       76u
#define ICON_CELL_H       84u
#define ICON_GLYPH_W      48u
#define ICON_GLYPH_H      48u
#define ICON_PAD          16u
#define ICON_LABEL_H      16u
#define ICON_BG           0xFF1D3557u
#define ICON_FILL         0xFF4C78A8u
#define ICON_SELECT       0xFFF4A261u
#define ICON_TEXT_BG      0xFF1D3557u

static icon_t icons[ICONS_MAX_ICONS];
static icon_stats_t icon_state;
static int icon_ready;
static uint32_t last_click_tick;

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

static icon_t *find_icon(uint32_t id) {
    for (size_t i = 0; i < ICONS_MAX_ICONS; i++) {
        if (icons[i].used && icons[i].id == id) {
            return &icons[i];
        }
    }
    return NULL;
}

static void clear_selection(void) {
    for (size_t i = 0; i < ICONS_MAX_ICONS; i++) {
        icons[i].selected = 0;
    }
    icon_state.selected_icon_id = 0;
}

static void draw_label(uint32_t x, uint32_t y, const char *text) {
    uint32_t len = 0;
    uint32_t box_w = ICON_CELL_W;
    while (text && text[len] != '\0' && len < ICONS_LABEL_MAX) {
        len++;
    }
    if (len == 0) {
        return;
    }
    fb_fill_rect(x, y, box_w, ICON_LABEL_H, ICON_TEXT_BG);
    fb_fill_rect(x + 2u, y + 2u, box_w - 4u, ICON_LABEL_H - 4u, ICON_BG);
}

static void draw_icon(const icon_t *icon, uint32_t x, uint32_t y) {
    uint32_t frame = icon->selected ? ICON_SELECT : 0xFF0B1320u;
    fb_fill_rect(x, y, ICON_CELL_W, ICON_CELL_H, frame);
    fb_fill_rect(x + 4u, y + 4u, ICON_GLYPH_W, ICON_GLYPH_H, icon->color);
    fb_fill_rect(x + 16u, y + 16u, 16u, 16u, 0xFFF1FAEEu);
    draw_label(x, y + ICON_GLYPH_H + 4u, icon->label);
}

void icons_init(void) {
    for (size_t i = 0; i < ICONS_MAX_ICONS; i++) {
        memset(&icons[i], 0, sizeof(icons[i]));
    }
    icon_state.icon_count = 0;
    icon_state.next_icon_id = 1;
    icon_state.selected_icon_id = 0;
    last_click_tick = 0;
    icon_ready = fb_ready() && wm_ready();
}

int icons_ready(void) {
    return icon_ready && fb_ready();
}

uint32_t icons_add(const char *label, const char *launch_cmd, uint32_t color) {
    icon_t *icon = NULL;
    for (size_t i = 0; i < ICONS_MAX_ICONS; i++) {
        if (!icons[i].used) {
            icon = &icons[i];
            break;
        }
    }
    if (!icon || !icons_ready()) {
        return 0;
    }
    memset(icon, 0, sizeof(*icon));
    icon->used = 1;
    icon->id = icon_state.next_icon_id++;
    icon->width = ICON_CELL_W;
    icon->height = ICON_CELL_H;
    icon->x = ICON_PAD + (uint32_t)((icon_state.icon_count % 4u) * (ICON_CELL_W + ICON_PAD));
    icon->y = ICON_PAD + (uint32_t)((icon_state.icon_count / 4u) * (ICON_CELL_H + ICON_PAD));
    icon->color = color;
    copy_text(icon->label, sizeof(icon->label), label ? label : "icon");
    copy_text(icon->launch_cmd, sizeof(icon->launch_cmd), launch_cmd ? launch_cmd : "");
    icon_state.icon_count++;
    return icon->id;
}

void icons_add_default_icons(void) {
    icons_add("Terminal", "shell", 0xFF2A9D8Fu);
    icons_add("Files", "files", 0xFFE9C46Au);
    icons_add("Editor", "editor", 0xFFE76F51u);
    icons_add("Calc", "calc", 0xFF4C78A8u);
    icons_add("Settings", "settings", 0xFF8E9AAFu);
    icons_add("System", "sysmon", 0xFFF4A261u);
}

void icons_render(void) {
    if (!icons_ready()) {
        return;
    }
    for (size_t i = 0; i < ICONS_MAX_ICONS; i++) {
        if (!icons[i].used) {
            continue;
        }
        draw_icon(&icons[i], icons[i].x, icons[i].y);
    }
}

static int in_icon(const icon_t *icon, int x, int y) {
    return icon &&
           x >= (int) icon->x && x < (int) (icon->x + icon->width) &&
           y >= (int) icon->y && y < (int) (icon->y + icon->height);
}

int icons_handle_click(int x, int y, uint8_t button_mask) {
    uint32_t now;
    if (!icons_ready()) {
        return -1;
    }
    if ((button_mask & 0x01) == 0) {
        return 0;
    }
    now = timer_get_ticks();
    for (size_t i = 0; i < ICONS_MAX_ICONS; i++) {
        if (icons[i].used && in_icon(&icons[i], x, y)) {
            if (icons[i].selected && icon_state.selected_icon_id == icons[i].id &&
                (now - last_click_tick) <= 30u) {
                last_click_tick = now;
                return icons_launch(icons[i].id) ? 2 : 1;
            }
            clear_selection();
            icons[i].selected = 1;
            icon_state.selected_icon_id = icons[i].id;
            last_click_tick = now;
            return 1;
        }
    }
    clear_selection();
    last_click_tick = now;
    return 0;
}

void icons_get_stats(icon_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = icon_state;
}

size_t icons_copy(icon_t *out, size_t max_icons) {
    size_t count = 0;
    if (!out) {
        return 0;
    }
    for (size_t i = 0; i < ICONS_MAX_ICONS && count < max_icons; i++) {
        if (!icons[i].used) {
            continue;
        }
        out[count++] = icons[i];
    }
    return count;
}

const icon_t *icons_get(uint32_t id) {
    return find_icon(id);
}

int icons_launch(uint32_t id) {
    const icon_t *icon = find_icon(id);
    if (!icon || icon->launch_cmd[0] == '\0') {
        return 0;
    }
    return shell_run_command(icon->launch_cmd) == 0 ? 1 : 0;
}

int icons_selected_launch_command(char *out, size_t out_size) {
    const icon_t *icon = NULL;
    if (!out || out_size == 0 || icon_state.selected_icon_id == 0) {
        return -1;
    }
    icon = find_icon(icon_state.selected_icon_id);
    if (!icon) {
        out[0] = '\0';
        return -1;
    }
    copy_text(out, out_size, icon->launch_cmd);
    return 0;
}
