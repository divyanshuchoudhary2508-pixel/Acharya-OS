/*
 * AcharyaOS - menu.c
 * ------------------
 * Phase 3, Feature 30: menus.
 *
 * This is a small registry-based menu layer. It keeps enough state to
 * describe top-level menus and paint a simple dropdown visualization. It is
 * intentionally compact: no input routing or nesting yet.
 */

#include "menu.h"
#include "framebuffer.h"
#include "window.h"
#include "kstring.h"

#define MENU_BAR_HEIGHT 22u
#define MENU_PADDING    4u

static menu_t menu_items[MENU_MAX_MENUS];
static menu_stats_t menu_state;
static int menu_is_ready;

static menu_t *menu_find(uint32_t id) {
    for (size_t i = 0; i < MENU_MAX_MENUS; i++) {
        if (menu_items[i].used && menu_items[i].id == id) {
            return &menu_items[i];
        }
    }
    return NULL;
}

static void menu_copy_text(char *dest, size_t size, const char *src) {
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

void menu_init(void) {
    for (size_t i = 0; i < MENU_MAX_MENUS; i++) {
        memset(&menu_items[i], 0, sizeof(menu_items[i]));
    }
    menu_state.menu_count = 0;
    menu_state.next_menu_id = 1;
    menu_state.active_menu_id = 0;
    menu_is_ready = fb_ready() && wm_ready();
}

int menu_ready(void) {
    return menu_is_ready && fb_ready();
}

uint32_t menu_create(uint32_t window_id, const char *title, uint32_t x, uint32_t y, uint32_t width) {
    menu_t *menu = NULL;

    if (!menu_ready() || width < 60u) {
        return 0;
    }

    for (size_t i = 0; i < MENU_MAX_MENUS; i++) {
        if (!menu_items[i].used) {
            menu = &menu_items[i];
            break;
        }
    }
    if (!menu) {
        return 0;
    }

    memset(menu, 0, sizeof(*menu));
    menu->used = 1;
    menu->id = menu_state.next_menu_id++;
    menu->window_id = window_id;
    menu->x = x;
    menu->y = y;
    menu->width = width;
    menu->item_height = 18u;
    menu->title_color = 0xFF334155u;
    menu->item_color = 0xFFE2E8F0u;
    menu->highlight_color = 0xFF94A3B8u;
    menu_copy_text(menu->title, sizeof(menu->title), title ? title : "menu");
    menu_state.menu_count++;
    if (menu_state.active_menu_id == 0) {
        menu_state.active_menu_id = menu->id;
    }
    return menu->id;
}

int menu_add_item(uint32_t menu_id, const char *text, uint32_t action_id) {
    menu_t *menu = menu_find(menu_id);
    if (!menu || menu->item_count >= MENU_ITEM_MAX) {
        return -1;
    }
    menu_copy_text(menu->items[menu->item_count].text, sizeof(menu->items[menu->item_count].text), text);
    menu->items[menu->item_count].action_id = action_id;
    menu->item_count++;
    return 0;
}

int menu_set_active(uint32_t menu_id) {
    if (!menu_find(menu_id)) {
        return -1;
    }
    menu_state.active_menu_id = menu_id;
    return 0;
}

static void menu_draw_one(const menu_t *menu) {
    uint32_t body_h;
    if (!menu || !menu->used) {
        return;
    }
    fb_fill_rect(menu->x, menu->y, menu->width, MENU_BAR_HEIGHT, menu->title_color);
    body_h = (uint32_t)(menu->item_count * menu->item_height);
    if (body_h > 0u) {
        fb_fill_rect(menu->x, menu->y + MENU_BAR_HEIGHT, menu->width, body_h, menu->item_color);
    }
    if (menu_state.active_menu_id == menu->id) {
        fb_fill_rect(menu->x, menu->y, menu->width, 2u, menu->highlight_color);
    }
}

void menu_render(void) {
    if (!menu_ready()) {
        return;
    }
    for (size_t i = 0; i < MENU_MAX_MENUS; i++) {
        if (!menu_items[i].used) {
            continue;
        }
        menu_draw_one(&menu_items[i]);
    }
}

void menu_demo_layout(void) {
    uint32_t file_menu;
    uint32_t edit_menu;

    menu_init();
    if (!menu_ready()) {
        return;
    }
    file_menu = menu_create(0, "File", 20u, 20u, 110u);
    edit_menu = menu_create(0, "Edit", 140u, 20u, 110u);
    (void) menu_add_item(file_menu, "New", 1u);
    (void) menu_add_item(file_menu, "Open", 2u);
    (void) menu_add_item(file_menu, "Save", 3u);
    (void) menu_add_item(edit_menu, "Copy", 4u);
    (void) menu_add_item(edit_menu, "Paste", 5u);
    menu_state.active_menu_id = file_menu;
    fb_clear(0xFF0F172Au);
    wm_render();
    menu_render();
}

void menu_get_stats(menu_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = menu_state;
}

size_t menu_copy_menus(menu_t *out, size_t max_menus) {
    size_t count = 0;
    if (!out) {
        return 0;
    }
    for (size_t i = 0; i < MENU_MAX_MENUS && count < max_menus; i++) {
        if (!menu_items[i].used) {
            continue;
        }
        out[count++] = menu_items[i];
    }
    return count;
}
