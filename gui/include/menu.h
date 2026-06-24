/*
 * AcharyaOS - menu.h
 * ------------------
 * Phase 3, Feature 30: menus.
 *
 * Menus are lightweight dropdown-style GUI data structures built on top of
 * the framebuffer/window layers. They start as a small registry and a demo
 * renderer so later GUI work can reuse a consistent menu API.
 */

#ifndef ACHARYAOS_MENU_H
#define ACHARYAOS_MENU_H

#include <stddef.h>
#include <stdint.h>

#define MENU_MAX_MENUS 8
#define MENU_TITLE_MAX  24
#define MENU_ITEM_MAX   8
#define MENU_ITEM_TEXT_MAX 24

typedef struct {
    char text[MENU_ITEM_TEXT_MAX];
    uint32_t action_id;
} menu_item_t;

typedef struct {
    int used;
    uint32_t id;
    uint32_t window_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t item_height;
    uint32_t title_color;
    uint32_t item_color;
    uint32_t highlight_color;
    char title[MENU_TITLE_MAX];
    size_t item_count;
    menu_item_t items[MENU_ITEM_MAX];
} menu_t;

typedef struct {
    size_t menu_count;
    uint32_t next_menu_id;
    uint32_t active_menu_id;
} menu_stats_t;

void menu_init(void);
int menu_ready(void);
uint32_t menu_create(uint32_t window_id, const char *title, uint32_t x, uint32_t y, uint32_t width);
int menu_add_item(uint32_t menu_id, const char *text, uint32_t action_id);
int menu_set_active(uint32_t menu_id);
void menu_render(void);
void menu_demo_layout(void);
void menu_get_stats(menu_stats_t *stats);
size_t menu_copy_menus(menu_t *out, size_t max_menus);

#endif /* ACHARYAOS_MENU_H */
