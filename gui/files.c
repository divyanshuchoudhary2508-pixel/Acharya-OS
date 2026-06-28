/*
 * AcharyaOS - files.c
 * -------------------
 * Feature 35: graphical file explorer.
 *
 * This is a lightweight graphical browser for the flat filesystem. It shows
 * file tiles, selection state, and a status summary. The implementation keeps
 * the layout simple so the subsystem stays easy to extend later with actual
 * navigation, folder trees, and text labels once a font renderer is in place.
 */

#include "files.h"
#include "framebuffer.h"
#include "fs.h"
#include "kstring.h"

#define FILES_BG            0xFF172A3Au
#define FILES_PANEL_BG      0xFF1F3245u
#define FILES_TILE_BG       0xFF2A3E52u
#define FILES_TILE_SEL      0xFFF4A261u
#define FILES_TILE_HEAD     0xFF4C78A8u
#define FILES_STATUS_BG     0xFF10151Fu
#define FILES_MAX_SHOW      16u
#define FILES_MARGIN        20u
#define FILES_TILE_W        92u
#define FILES_TILE_H        76u
#define FILES_TILE_GAP      14u
#define FILES_HEADER_H      28u
#define FILES_STATUS_H      24u

static files_stats_t g_stats;
static int g_ready;

static void files_draw_tile(uint32_t x, uint32_t y, uint32_t color, int selected) {
    uint32_t frame = selected ? FILES_TILE_SEL : FILES_TILE_HEAD;
    fb_fill_rect(x, y, FILES_TILE_W, FILES_TILE_H, frame);
    fb_fill_rect(x + 4u, y + 4u, FILES_TILE_W - 8u, FILES_TILE_H - 8u, color);
    fb_fill_rect(x + 26u, y + 20u, 20u, 20u, 0xFFE9ECEFu);
}

void files_init(void) {
    fs_info_t info;
    g_ready = 1;
    memset(&g_stats, 0, sizeof(g_stats));
    fs_get_info(&info);
    g_stats.ready = fs_mounted();
}

int files_ready(void) {
    return g_ready && fs_mounted();
}

void files_render(void) {
    fs_file_entry_t files[FS_FILES_MAX];
    size_t count;
    uint32_t x;
    uint32_t y;
    uint32_t screen_w;
    uint32_t screen_h;
    fb_info_t fb;

    if (!files_ready()) {
        return;
    }

    fb_get_info(&fb);
    screen_w = fb.width;
    screen_h = fb.height;
    fb_fill_rect(0u, 0u, screen_w, screen_h, FILES_BG);
    fb_fill_rect(FILES_MARGIN, FILES_MARGIN, screen_w - (FILES_MARGIN * 2u), screen_h - (FILES_MARGIN * 2u), FILES_PANEL_BG);
    fb_fill_rect(FILES_MARGIN, FILES_MARGIN, screen_w - (FILES_MARGIN * 2u), FILES_HEADER_H, FILES_TILE_HEAD);

    count = fs_list_files(files, FS_FILES_MAX);
    if (count > FILES_MAX_SHOW) {
        count = FILES_MAX_SHOW;
    }

    x = FILES_MARGIN + FILES_TILE_GAP;
    y = FILES_MARGIN + FILES_HEADER_H + FILES_TILE_GAP;
    for (size_t i = 0; i < count; i++) {
        files_draw_tile(x, y, (files[i].used != 0) ? FILES_TILE_BG : FILES_STATUS_BG, i == g_stats.selected_index);
        x += FILES_TILE_W + FILES_TILE_GAP;
        if (x + FILES_TILE_W > screen_w - FILES_MARGIN) {
            x = FILES_MARGIN + FILES_TILE_GAP;
            y += FILES_TILE_H + FILES_TILE_GAP;
        }
    }

    fb_fill_rect(FILES_MARGIN, screen_h - FILES_MARGIN - FILES_STATUS_H, screen_w - (FILES_MARGIN * 2u), FILES_STATUS_H, FILES_STATUS_BG);
    g_stats.file_count = count;
    g_stats.render_count++;
}

void files_get_stats(files_stats_t *stats) {
    if (!stats) {
        return;
    }
    *stats = g_stats;
}

