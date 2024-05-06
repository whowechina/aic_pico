/*
 * Light (WS2812 + LED) control
 * WHowe <github.com/whowechina>
 * 
 */

#include "light.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

static uint32_t rgb_buf[64];
static uint8_t led_gpio[] = LED_DEF;
#define RGB_NUM (sizeof(rgb_buf) / sizeof(rgb_buf[0]))
#define LED_NUM (sizeof(led_gpio))
static uint8_t led_buf[LED_NUM];


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

static inline uint32_t apply_level_by(uint32_t color, uint8_t level)
{
    unsigned r = (color >> 16) & 0xff;
    unsigned g = (color >> 8) & 0xff;
    unsigned b = color & 0xff;

    r = r * level / 255;
    g = g * level / 255;
    b = b * level / 255;

    return r << 16 | g << 8 | b;
}

static uint8_t curr_level = 0;
static inline uint32_t apply_level(uint32_t color)
{
    return apply_level_by(color, curr_level);
}

/* 6 segment regular hsv color wheel, better color cycle
 * https://www.arnevogel.com/rgb-rainbow/
 * https://www.instructables.com/How-to-Make-Proper-Rainbow-and-Random-Colors-With-/
 */
#define COLOR_WHEEL_SIZE 256
static uint32_t color_wheel[COLOR_WHEEL_SIZE];
static void generate_color_wheel()
{
    for (int i = 0; i < COLOR_WHEEL_SIZE; i++) {
        color_wheel[i] = rgb32_from_hsv(i, 208, 255);
    }
}

#define RAINBOW_PITCH 37
#define RAINBOW_MIN_SPEED 1
static uint32_t curr_speed = RAINBOW_MIN_SPEED;

static void rainbow_update()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 33333) { // no faster than 30Hz
        return;
    }
    last = now;

    static uint32_t rotator = 0;
    rotator = (rotator + curr_speed) % COLOR_WHEEL_SIZE;

    for (int i = 0; i < RGB_NUM; i++) {
        uint32_t index = (rotator + RAINBOW_PITCH * i) % COLOR_WHEEL_SIZE;
        rgb_buf[i] = apply_level(color_wheel[index]);
    }

    for (int i = 0; i < LED_NUM; i++) {
        uint32_t index = (rotator + RAINBOW_PITCH * 2 * i) % COLOR_WHEEL_SIZE;
        led_buf[i] = apply_level(color_wheel[index]) & 0xff;
    }
}

void light_stimulate()
{
    curr_speed = 48;
    curr_level = aic_cfg->light.max;
}

static void rainbow_fade()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 200000) {
        return;
    }
    last = now;

    if (curr_speed > RAINBOW_MIN_SPEED) {
        curr_speed = curr_speed * 90 / 100;
    }
    if (curr_level > aic_cfg->light.min) {
        curr_level -= (curr_level - aic_cfg->light.min) / 10 + 1;
    } else if (curr_level < aic_cfg->light.min) {
        curr_level += (aic_cfg->light.min - curr_level) / 10 + 1;
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

    for (int i = 0; i < RGB_NUM; i++) {
        uint32_t color = aic_cfg->light.rgb ? rgb_buf[i] << 8u : 0;
        pio_sm_put_blocking(pio0, 0, color);
    }

    for (int i = 0; i < LED_NUM; i++) {
        uint8_t level = aic_cfg->light.led ? led_buf[i] : 0;
        pwm_set_gpio_level(led_gpio[i], level);
    }
}

void light_set_color(unsigned index, uint32_t color)
{
    if (index >= RGB_NUM) {
        return;
    }
    rgb_buf[index] = apply_level(color);
}

void light_set_color_all(uint32_t color)
{
    for (int i = 0; i < RGB_NUM; i++) {
        rgb_buf[i] = apply_level_by(color, aic_cfg->light.max);
    }
}

static uint64_t last_hid = 0;
void light_hid_light(uint8_t r, uint8_t g, uint8_t b)
{
    light_set_color_all(rgb32(r, g, b, false));
    last_hid = time_us_64();
}

void light_set_brg(unsigned index, const uint8_t *brg_array, size_t num)
{
    if (index >= RGB_NUM) {
        return;
    }
    if (index + num > RGB_NUM) {
        num = RGB_NUM - index;
    }
    for (int i = 0; i < num; i++) {
        uint8_t b = brg_array[i * 3 + 0];
        uint8_t r = brg_array[i * 3 + 1];
        uint8_t g = brg_array[i * 3 + 2];
        rgb_buf[index + i] = apply_level(rgb32(r, g, b, false));
    }
}

void light_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);
    gpio_set_drive_strength(RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, RGB_PIN, 800000, false);

    for (int i = 0; i < LED_NUM; i++) {
        gpio_init(led_gpio[i]);
        gpio_set_dir(led_gpio[i], GPIO_OUT);
        gpio_set_function(led_gpio[i], GPIO_FUNC_PWM);

        int slice = pwm_gpio_to_slice_num(led_gpio[i]);

        pwm_config cfg = pwm_get_default_config();
        pwm_config_set_clkdiv(&cfg, 4.f);
        pwm_init(slice, &cfg, true);
    }

    curr_level = aic_cfg->light.min;
    generate_color_wheel();
}

static bool rainbow = true;
void light_set_rainbow(bool enable)
{
    rainbow = enable;
}

void light_update()
{
    if (rainbow && (time_us_64() > last_hid + 1000000)) {
        rainbow_update();
    }
    rainbow_fade();
    drive_led();
}
