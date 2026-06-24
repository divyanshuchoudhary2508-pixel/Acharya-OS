/*
 * AcharyaOS - button.c
 * --------------------
 * Phase 3, Feature 29: buttons.
 */

#include "button.h"
#include "window.h"
#include "framebuffer.h"
#include "kstring.h"

#define BTN_PADDING 4u

static btn_t btn_buttons[BTN_MAX_BUTTONS];
static btn_stats_t btn_state;

static btn_t *btn_find(uint32_t id) {
    for (size_t i = 0; i < BTN_MAX_BUTTONS; i++) {
        if (btn_buttons[i].used && btn_buttons[i].id == id) {
            return &btn_buttons[i];
        }
    }
    return NULL;
}

static void btn_copy_label(char dest[BTN_LABEL_MAX], const char *src) {
    size_t i = 0;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    while (i + 1 < BTN_LABEL_MAX && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void btn_draw_one(const btn_t *btn) {
    uint32_t fill;
    uint32_t inner_x;
    uint32_t inner_y;
    uint32_t inner_w;
    uint32_t inner_h;

    if (!btn || !btn->used) {
        return;
    }

    fill = btn->pressed ? btn->pressed_color : btn->fill_color;
    fb_fill_rect(btn->x, btn->y, btn->width, btn->height, btn->border_color);

    if (btn->width <= 2u || btn->height <= 2u) {
        return;
    }

    inner_x = btn->x + 1u;
    inner_y = btn->y + 1u;
    inner_w = btn->width - 2u;
    inner_h = btn->height - 2u;
    fb_fill_rect(inner_x, inner_y, inner_w, inner_h, fill);

    if (inner_w > (BTN_PADDING * 2u) && inner_h > (BTN_PADDING * 2u)) {
        fb_fill_rect(inner_x + BTN_PADDING, inner_y + BTN_PADDING,
                     inner_w - (BTN_PADDING * 2u), inner_h - (BTN_PADDING * 2u),
                     btn->text_color);
    }
}

void btn_init(void) {
    for (size_t i = 0; i < BTN_MAX_BUTTONS; i++) {
        memset(&btn_buttons[i], 0, sizeof(btn_buttons[i]));
    }
    btn_state.button_count = 0;
    btn_state.next_button_id = 1;
    btn_state.pressed_button_id = 0;
}

uint32_t btn_create(uint32_t window_id, const char *label, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    btn_t *btn = NULL;

    if (!wm_ready() || !fb_ready() || width < 12u || height < 12u) {
        return 0;
    }

    for (size_t i = 0; i < BTN_MAX_BUTTONS; i++) {
        if (!btn_buttons[i].used) {
            btn = &btn_buttons[i];
            break;
        }
    }
    if (!btn) {
        return 0;
    }

    memset(btn, 0, sizeof(*btn));
    btn->used = 1;
    btn->id = btn_state.next_button_id++;
    btn->window_id = window_id;
    btn->x = x;
    btn->y = y;
    btn->width = width;
    btn->height = height;
    btn->fill_color = 0xFFB9C2CFu;
    btn->border_color = 0xFF2A2E35u;
    btn->pressed_color = 0xFF7AA2F7u;
    btn->text_color = 0xFFF4F7FAu;
    btn->pressed = 0;
    btn_copy_label(btn->label, label ? label : "button");

    btn_state.button_count++;
    return btn->id;
}

int btn_set_pressed(uint32_t id, int pressed) {
    btn_t *btn = btn_find(id);
    if (!btn) {
        return -1;
    }
    btn->pressed = pressed ? 1 : 0;
    btn_state.pressed_button_id = pressed ? id : 0;
    return 0;
}

void btn_render(void) {
    for (size_t i = 0; i < BTN_MAX_BUTTONS; i++) {
        if (!btn_buttons[i].used) {
            continue;
        }
        btn_draw_one(&btn_buttons[i]);
    }
}

void btn_demo_layout(void) {
    uint32_t shell = 0;
    uint32_t notes = 0;
    uint32_t power = 0;

    if (!wm_ready() || !fb_ready()) {
        return;
    }

    wm_demo_layout();
    shell = btn_create(1, "Shell", 38u, 58u, 52u, 22u);
    notes = btn_create(2, "Notes", 208u, 86u, 56u, 22u);
    power = btn_create(3, "Power", 112u, 148u, 56u, 22u);
    (void) shell;
    (void) notes;
    (void) power;

    if (shell) {
        btn_set_pressed(shell, 1);
    }
    btn_render();
}

void btn_get_stats(btn_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = btn_state;
}

size_t btn_copy_buttons(btn_t *out, size_t max_buttons) {
    size_t count;
    if (!out) {
        return 0;
    }
    count = 0;
    for (size_t i = 0; i < BTN_MAX_BUTTONS && count < max_buttons; i++) {
        if (!btn_buttons[i].used) {
            continue;
        }
        out[count++] = btn_buttons[i];
    }
    return count;
}
