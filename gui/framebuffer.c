/*
 * AcharyaOS - framebuffer.c
 * -------------------------
 * Phase 3, Feature 26: framebuffer graphics.
 *
 * This implementation uses a small in-kernel software framebuffer for now.
 * That gives us a stable graphics API and a place to build rendering code
 * before the boot path is extended to consume a real framebuffer from the
 * loader. The buffer is intentionally modest in size so it stays friendly to
 * the project's resource limits.
 */

#include "framebuffer.h"
#include "kstring.h"

#define FB_WIDTH 320u
#define FB_HEIGHT 200u
#define FB_BYTES_PER_PIXEL 4u

static uint32_t fb_backing[FB_WIDTH * FB_HEIGHT];
static fb_info_t fb_state;

static void fb_sync_state(void) {
    fb_state.width = FB_WIDTH;
    fb_state.height = FB_HEIGHT;
    fb_state.pitch = FB_WIDTH * FB_BYTES_PER_PIXEL;
    fb_state.bytes_per_pixel = FB_BYTES_PER_PIXEL;
    fb_state.pixel_count = FB_WIDTH * FB_HEIGHT;
    fb_state.ready = 1;
}

void fb_init(void) {
    fb_state.ready = 0;
    fb_state.background = 0xFF000000u;
    fb_state.foreground = 0xFFFFFFFFu;
    fb_sync_state();
    fb_clear(fb_state.background);
}

int fb_ready(void) {
    return fb_state.ready;
}

void fb_get_info(fb_info_t *info) {
    if (!info) {
        return;
    }
    *info = fb_state;
}

void fb_clear(uint32_t color) {
    for (size_t i = 0; i < (size_t) fb_state.pixel_count; i++) {
        fb_backing[i] = color;
    }
    fb_state.background = color;
}

int fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_state.ready || x >= fb_state.width || y >= fb_state.height) {
        return -1;
    }
    fb_backing[(y * fb_state.width) + x] = color;
    return 0;
}

int fb_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    uint32_t x_end;
    uint32_t y_end;

    if (!fb_state.ready || width == 0 || height == 0) {
        return -1;
    }

    x_end = x + width;
    y_end = y + height;
    if (x >= fb_state.width || y >= fb_state.height) {
        return -1;
    }
    if (x_end > fb_state.width || y_end > fb_state.height) {
        return -1;
    }

    for (uint32_t row = y; row < y_end; row++) {
        for (uint32_t col = x; col < x_end; col++) {
            fb_backing[(row * fb_state.width) + col] = color;
        }
    }
    return 0;
}

void fb_demo_pattern(void) {
    uint32_t tile = 20u;

    if (!fb_state.ready) {
        return;
    }

    fb_clear(0xFF101820u);
    for (uint32_t y = 0; y < fb_state.height; y += tile) {
        for (uint32_t x = 0; x < fb_state.width; x += tile) {
            uint32_t odd = ((x / tile) + (y / tile)) & 1u;
            uint32_t color = odd ? 0xFF2D8CFFu : 0xFF16324Fu;
            fb_fill_rect(x, y, tile, tile, color);
        }
    }
    fb_fill_rect(20u, 20u, 120u, 40u, 0xFFE8B04Cu);
    fb_fill_rect(160u, 20u, 120u, 40u, 0xFF70C1B3u);
    fb_fill_rect(20u, 80u, 260u, 30u, 0xFFD94F70u);
}

const uint32_t *fb_pixels(void) {
    return fb_backing;
}
