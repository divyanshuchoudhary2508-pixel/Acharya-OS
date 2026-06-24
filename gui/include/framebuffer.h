/*
 * AcharyaOS - framebuffer.h
 * -------------------------
 * Phase 3, Feature 26: framebuffer graphics.
 *
 * This module owns a very small graphics surface abstraction that can later
 * be backed by a real bootloader-provided framebuffer. For now it gives the
 * kernel a disciplined place to build pixel graphics without entangling the
 * text console or VGA text mode driver.
 */

#ifndef ACHARYAOS_FRAMEBUFFER_H
#define ACHARYAOS_FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bytes_per_pixel;
    uint32_t pixel_count;
    int ready;
    uint32_t background;
    uint32_t foreground;
} fb_info_t;

void fb_init(void);
int fb_ready(void);
void fb_get_info(fb_info_t *info);
void fb_clear(uint32_t color);
int fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
int fb_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_demo_pattern(void);
const uint32_t *fb_pixels(void);

#endif /* ACHARYAOS_FRAMEBUFFER_H */
