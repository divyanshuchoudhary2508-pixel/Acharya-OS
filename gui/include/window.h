/*
 * AcharyaOS - window.h
 * --------------------
 * Phase 3, Feature 27: window manager.
 *
 * The window manager keeps a very small registry of rectangular windows and
 * knows how to paint them onto the framebuffer. It does not yet do input
 * focus, dragging, or overlapping z-order policy beyond registration order.
 * The goal here is to create the core abstraction cleanly.
 */

#ifndef ACHARYAOS_WINDOW_H
#define ACHARYAOS_WINDOW_H

#include <stddef.h>
#include <stdint.h>

#define WM_MAX_WINDOWS 8
#define WM_TITLE_MAX   24

typedef struct {
    int used;
    uint32_t id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t border_color;
    uint32_t title_color;
    uint32_t client_color;
    char title[WM_TITLE_MAX];
} wm_window_t;

typedef struct {
    size_t window_count;
    uint32_t next_window_id;
    uint32_t active_window_id;
} wm_stats_t;

void wm_init(void);
int wm_ready(void);
uint32_t wm_create_window(const char *title, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
int wm_set_active(uint32_t id);
void wm_render(void);
void wm_demo_layout(void);
void wm_get_stats(wm_stats_t *stats);
size_t wm_copy_windows(wm_window_t *out, size_t max_windows);

#endif /* ACHARYAOS_WINDOW_H */
