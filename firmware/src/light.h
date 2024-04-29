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

void light_init();
void light_update();

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v);

void light_set_color(unsigned index, uint32_t color);
void light_set_color_all(uint32_t color);
void light_hid_light(uint8_t r, uint8_t g, uint8_t b);

void light_set_rainbow(bool enable);
void light_stimulate();

#endif
