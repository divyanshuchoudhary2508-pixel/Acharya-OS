/*
 * AcharyaOS - button.h
 * --------------------
 * Phase 3, Feature 29: buttons.
 *
 * Buttons are lightweight GUI controls drawn on top of the framebuffer and
 * window manager. They store geometry and colors now; text labels remain
 * metadata until a font renderer exists.
 */

#ifndef ACHARYAOS_BUTTON_H
#define ACHARYAOS_BUTTON_H

#include <stddef.h>
#include <stdint.h>

#define BTN_MAX_BUTTONS 16
#define BTN_LABEL_MAX   24

typedef struct {
    int used;
    uint32_t id;
    uint32_t window_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t fill_color;
    uint32_t border_color;
    uint32_t pressed_color;
    uint32_t text_color;
    int pressed;
    char label[BTN_LABEL_MAX];
} btn_t;

typedef struct {
    size_t button_count;
    uint32_t next_button_id;
    uint32_t pressed_button_id;
} btn_stats_t;

void btn_init(void);
uint32_t btn_create(uint32_t window_id, const char *label, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
int btn_set_pressed(uint32_t id, int pressed);
void btn_render(void);
void btn_demo_layout(void);
void btn_get_stats(btn_stats_t *stats);
size_t btn_copy_buttons(btn_t *out, size_t max_buttons);

#endif /* ACHARYAOS_BUTTON_H */
