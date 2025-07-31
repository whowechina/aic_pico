/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 */

#ifndef RGB_H
#define RGB_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

typedef enum {
    GRB = 0,
    BRG = 1,
    RGB = 2,
    RBG = 3,
} rgb_order_t;

void light_init();
void light_update();
void light_set_rgb_order(rgb_order_t order);
const char *light_get_rgb_order_string(rgb_order_t order);

uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v);

void light_fade(uint32_t color, uint32_t fading_ms);
void light_fade_n(int repeat, int count, ...);
void light_fade_s(const char *pattern);

void light_rainbow(int8_t speed, uint32_t smooth_ms, uint8_t level);

#endif
