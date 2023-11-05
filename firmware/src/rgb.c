/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 * 
 */

#include "rgb.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static uint32_t rgb_buf[16];

#define _MAP_LED(x) _MAKE_MAPPER(x)
#define _MAKE_MAPPER(x) MAP_LED_##x
#define MAP_LED_RGB { c1 = r; c2 = g; c3 = b; }
#define MAP_LED_GRB { c1 = g; c2 = r; c3 = b; }

#define REMAP_BUTTON_RGB _MAP_LED(BUTTON_RGB_ORDER)
#define REMAP_TT_RGB _MAP_LED(TT_RGB_ORDER)

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }
    
    return (c1 << 16) | (c2 << 8) | (c3 << 0);    
}

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t region, remainder, p, q, t;

    if (s == 0) {
        return v << 16 | v << 8 | v;
    }

    region = h / 43;
    remainder = (h % 43) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            return v << 16 | t << 8 | p;
        case 1:
            return q << 16 | v << 8 | p;
        case 2:
            return p << 16 | v << 8 | t;
        case 3:
            return p << 16 | q << 8 | v;
        case 4:
            return t << 16 | p << 8 | v;
        default:
            return v << 16 | p << 8 | q;
    }
}

static inline uint32_t apply_level(uint32_t color)
{
    unsigned r = (color >> 16) & 0xff;
    unsigned g = (color >> 8) & 0xff;
    unsigned b = color & 0xff;

    r = r * aic_cfg->led.level / 255;
    g = g * aic_cfg->led.level / 255;
    b = b * aic_cfg->led.level / 255;

    return r << 16 | g << 8 | b;
}

/* 6 segment regular hsv color wheel, better color cycle
 * https://www.arnevogel.com/rgb-rainbow/
 * https://www.instructables.com/How-to-Make-Proper-Rainbow-and-Random-Colors-With-/
 */
#define COLOR_WHEEL_SIZE 256
static uint32_t color_wheel[COLOR_WHEEL_SIZE];
static void generate_color_wheel()
{
    static uint8_t old_level = 0;
    if (old_level == aic_cfg->led.level) {
        return;
    }
    old_level = aic_cfg->led.level;

    for (int i = 0; i < COLOR_WHEEL_SIZE; i++) {
        color_wheel[i] = rgb32_from_hsv(i, 208, 255);
        color_wheel[i] = apply_level(color_wheel[i]);
    }
}

#define RAINBOW_PITCH 37
#define RAINBOW_MIN_SPEED 1
static uint32_t rainbow_speed = RAINBOW_MIN_SPEED;

static void rainbow_update()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 33333) { // no faster than 30Hz
        return;
    }
    last = now;

    static uint32_t rotator = 0;
    rotator = (rotator + rainbow_speed) % COLOR_WHEEL_SIZE;

    for (int i = 0; i < ARRAY_SIZE(rgb_buf); i++) {
        uint32_t index = (rotator + RAINBOW_PITCH * i) % COLOR_WHEEL_SIZE;
        rgb_buf[i] = color_wheel[index];
    }
}

void rgb_set_rainbow_speed(uint8_t speed)
{
    rainbow_speed = speed / 8;
}

static void rainbow_speed_down()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 200000) {
        return;
    }
    last = now;

    if (rainbow_speed > RAINBOW_MIN_SPEED) {
        rainbow_speed = rainbow_speed * 95 / 100;
    }
}

static void drive_led()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 4000) { // no faster than 250Hz
        return;
    }
    last = now;

    for (int i = 0; i < ARRAY_SIZE(rgb_buf); i++) {
        pio_sm_put_blocking(pio0, 0, rgb_buf[i] << 8u);
    }
}

void rgb_set_color(unsigned index, uint32_t color)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    rgb_buf[index] = apply_level(color);
}

void rgb_set_color_all(uint32_t color)
{
    for (int i = 0; i < ARRAY_SIZE(rgb_buf); i++) {
        rgb_buf[i] = apply_level(color);
    }
}

void rgb_set_brg(unsigned index, const uint8_t *brg_array, size_t num)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    if (index + num > ARRAY_SIZE(rgb_buf)) {
        num = ARRAY_SIZE(rgb_buf) - index;
    }
    for (int i = 0; i < num; i++) {
        uint8_t b = brg_array[i * 3 + 0];
        uint8_t r = brg_array[i * 3 + 1];
        uint8_t g = brg_array[i * 3 + 2];
        rgb_buf[index + i] = apply_level(rgb32(r, g, b, false));
    }
}

void rgb_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);
    gpio_set_drive_strength(RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, RGB_PIN, 800000, false);
    
    generate_color_wheel();
}

void rgb_update()
{
    generate_color_wheel();

    rainbow_update();
    rainbow_speed_down();
    drive_led();
}
