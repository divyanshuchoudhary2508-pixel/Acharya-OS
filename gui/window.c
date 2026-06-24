/*
 * AcharyaOS - window.c
 * --------------------
 * Phase 3, Feature 27: window manager.
 */

#include "window.h"
#include "framebuffer.h"
#include "kstring.h"

#define WM_BORDER_THICKNESS 2u
#define WM_TITLE_HEIGHT 18u

static wm_window_t wm_windows[WM_MAX_WINDOWS];
static wm_stats_t wm_state;
static int wm_is_ready;

static wm_window_t *wm_find(uint32_t id) {
    for (size_t i = 0; i < WM_MAX_WINDOWS; i++) {
        if (wm_windows[i].used && wm_windows[i].id == id) {
            return &wm_windows[i];
        }
    }
    return NULL;
}

static void wm_copy_title(char dest[WM_TITLE_MAX], const char *src) {
    size_t i = 0;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    while (i + 1 < WM_TITLE_MAX && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void wm_draw_window(const wm_window_t *win) {
    uint32_t x1;
    uint32_t y1;
    uint32_t border;
    uint32_t client_x;
    uint32_t client_y;
    uint32_t client_w;
    uint32_t client_h;

    if (!win || !win->used) {
        return;
    }

    x1 = win->x + win->width;
    y1 = win->y + win->height;
    border = win->border_color;

    fb_fill_rect(win->x, win->y, win->width, win->height, border);

    client_x = win->x + WM_BORDER_THICKNESS;
    client_y = win->y + WM_TITLE_HEIGHT;
    client_w = win->width > (WM_BORDER_THICKNESS * 2u) ? win->width - (WM_BORDER_THICKNESS * 2u) : 0u;
    client_h = win->height > (WM_TITLE_HEIGHT + WM_BORDER_THICKNESS) ?
        win->height - WM_TITLE_HEIGHT - WM_BORDER_THICKNESS : 0u;

    if (client_w > 0u && client_h > 0u) {
        fb_fill_rect(client_x, client_y, client_w, client_h, win->client_color);
    }

    if (x1 > win->x && y1 > win->y) {
        uint32_t title_w = win->width > (WM_BORDER_THICKNESS * 2u) ? win->width - (WM_BORDER_THICKNESS * 2u) : 0u;
        if (title_w > 0u) {
            fb_fill_rect(win->x + WM_BORDER_THICKNESS, win->y + WM_BORDER_THICKNESS,
                         title_w, WM_TITLE_HEIGHT - WM_BORDER_THICKNESS, win->title_color);
        }
    }
}

void wm_init(void) {
    for (size_t i = 0; i < WM_MAX_WINDOWS; i++) {
        memset(&wm_windows[i], 0, sizeof(wm_windows[i]));
    }
    wm_state.window_count = 0;
    wm_state.next_window_id = 1;
    wm_state.active_window_id = 0;
    wm_is_ready = fb_ready();
}

int wm_ready(void) {
    return wm_is_ready && fb_ready();
}

uint32_t wm_create_window(const char *title, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    wm_window_t *win = NULL;

    if (!wm_ready() || width < 40u || height < 30u) {
        return 0;
    }

    for (size_t i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!wm_windows[i].used) {
            win = &wm_windows[i];
            break;
        }
    }
    if (!win) {
        return 0;
    }

    memset(win, 0, sizeof(*win));
    win->used = 1;
    win->id = wm_state.next_window_id++;
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->border_color = 0xFF1D1F24u;
    win->title_color = 0xFF4C78A8u;
    win->client_color = 0xFFE7E9EDu;
    wm_copy_title(win->title, title ? title : "window");

    wm_state.window_count++;
    if (wm_state.active_window_id == 0) {
        wm_state.active_window_id = win->id;
    }
    return win->id;
}

int wm_set_active(uint32_t id) {
    if (!wm_find(id)) {
        return -1;
    }
    wm_state.active_window_id = id;
    return 0;
}

void wm_render(void) {
    if (!wm_ready()) {
        return;
    }

    fb_clear(0xFF101820u);
    for (size_t i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!wm_windows[i].used) {
            continue;
        }
        wm_draw_window(&wm_windows[i]);
    }

    if (wm_state.active_window_id != 0) {
        wm_window_t *active = wm_find(wm_state.active_window_id);
        if (active) {
            fb_fill_rect(active->x, active->y, active->width, 3u, 0xFFF2C14Eu);
        }
    }
}

void wm_demo_layout(void) {
    wm_init();
    if (!wm_ready()) {
        return;
    }
    (void) wm_create_window("shell", 20u, 20u, 150u, 90u);
    (void) wm_create_window("notes", 190u, 35u, 110u, 120u);
    (void) wm_create_window("monitor", 70u, 130u, 180u, 55u);
    wm_state.active_window_id = 1;
    wm_render();
}

void wm_get_stats(wm_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = wm_state;
}

size_t wm_copy_windows(wm_window_t *out, size_t max_windows) {
    size_t count;
    if (!out) {
        return 0;
    }
    count = 0;
    for (size_t i = 0; i < WM_MAX_WINDOWS && count < max_windows; i++) {
        if (!wm_windows[i].used) {
            continue;
        }
        out[count++] = wm_windows[i];
    }
    return count;
}
