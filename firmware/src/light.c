/*
 * Light (WS2812 + LED) control
 * WHowe <github.com/whowechina>
 * 
 */

#include "light.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

static uint32_t rgb_buf[64];
static uint8_t led_gpio[] = LED_DEF;
#define RGB_NUM (sizeof(rgb_buf) / sizeof(rgb_buf[0]))
#define LED_NUM (sizeof(led_gpio))
static uint16_t led_buf[LED_NUM];
static int rgb_dma;
static dma_channel_config rgb_dma_cfg;

static inline uint32_t rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }
    
    return (c1 << 16) | (c2 << 8) | (c3 << 0);    
}

/* translate RGB to local RGB channel order */
static inline uint32_t rgb2local(uint32_t color, bool gamma_fix)
{
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = color & 0xff;

#if BUTTON_RGB_ORDER == GRB
    return rgb32(g, r, b, gamma_fix);
#else
    return rgb32(r, g, b, gamma_fix);
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

static inline uint32_t apply_level(uint32_t color, uint8_t level)
{
    unsigned c1 = (color >> 16) & 0xff;
    unsigned c2 = (color >> 8) & 0xff;
    unsigned c3 = color & 0xff;

    c1 = c1 * level / 255;
    c2 = c2 * level / 255;
    c3 = c3 * level / 255;

    return c1 << 16 | c2 << 8 | c3;
}

static inline uint16_t apply_gray_level(uint8_t gray, uint8_t level)
{
    return gray * level;
}

/* 6 segment regular hsv color wheel, better color cycle
 * https://www.arnevogel.com/rgb-rainbow/
 * https://www.instructables.com/How-to-Make-Proper-Rainbow-and-Random-Colors-With-/
 */
#define COLOR_WHEEL_SIZE 256
static uint32_t color_wheel[COLOR_WHEEL_SIZE];
static uint8_t gray_wheel[COLOR_WHEEL_SIZE];

static void generate_color_wheel()
{
    for (int i = 0; i < COLOR_WHEEL_SIZE; i++) {
        color_wheel[i] = rgb32_from_hsv(i, 208, 255);
        gray_wheel[i] = i < 128 ? i : (255 - i);
    }
}

static inline uint8_t lerp8b(uint8_t a, uint8_t b, uint8_t t)
{
    return a + (b - a) * t / 255;
}

static uint32_t lerp(uint32_t a, uint32_t b, int pos, int range)
{
    uint8_t t = pos * 255 / range;
    uint32_t c1 = lerp8b((a >> 16) & 0xff, (b >> 16) & 0xff, t);
    uint32_t c2 = lerp8b((a >> 8) & 0xff, (b >> 8) & 0xff, t);
    uint32_t c3 = lerp8b(a & 0xff, b & 0xff, t);
    return c1 << 16 | c2 << 8 | c3;
}

static enum {
    MODE_FADE,
    MODE_RAINBOW,
} light_mode = MODE_RAINBOW;

static struct {
    int repeat;
    int step_num;
    int curr_step;
    uint32_t color;
    int elapsed;
    struct {
        uint32_t from;
        uint32_t to;
        int duration;
    } steps[32];
} fading;

void light_fade_n(int repeat, int count, ...)
{
    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        fading.steps[i].from = i == 0 ? fading.color : fading.steps[i - 1].to;
        fading.steps[i].to = rgb2local(va_arg(args, uint32_t), true);
        fading.steps[i].duration = va_arg(args, int);
    }

    va_end(args);

    fading.repeat = repeat;
    fading.step_num = count;
    fading.curr_step = 0;
    fading.elapsed = 0;

    light_mode = MODE_FADE;
}

void light_fade(uint32_t color, uint32_t fading_ms)
{
    light_fade_n(1, 1, color, fading_ms);
}

static uint32_t htoi(const char *s)
{
    uint32_t result = 0;

    for (; *s != '\0'; s++) {
        char c = *s;
        result <<= 4;
        if ((c >= '0') && (c <= '9')) {
            result |= c - '0';
        } else if ((c >= 'A') && (c <= 'F')) {
            result |= c - 'A' + 10;
        } else if ((c >= 'a') && (c <= 'f')) {
            result |= c - 'a' + 10;
        } else {
            break;
        }
    }

    return result;
}

static int parse_integers(const char *str, int32_t *output)
{
    static char patt[256];
    strncpy(patt, str, sizeof(patt) - 1);
    patt[sizeof(patt) - 1] = '\0';

    int count = 0;
    for (char *token = strtok(patt, ", "); token; token = strtok(NULL, ", ")) {
        if (token[0] == '#') {
            output[count] = htoi(token + 1);
        } else {
            output[count] = atoi(token);
        }
        count++;
    }

    return count;
}

void light_fade_s(const char *pattern)
{
    static int32_t param[100];

    if (!pattern) {
        return;
    }

    int param_num = parse_integers(pattern, param);

    if ((param_num < 3) || (param_num % 2 != 1)) {
        return;
    }
    
    fading.repeat = param[0];
    fading.step_num = (param_num - 1) / 2;
    for (int i = 0; i < fading.step_num; i++) {
        fading.steps[i].from = i == 0 ? fading.color : fading.steps[i - 1].to;
        fading.steps[i].to = rgb2local(param[i * 2 + 1], true);
        fading.steps[i].duration = param[i * 2 + 2];
    }
    fading.curr_step = 0;
    fading.elapsed = 0;

    light_mode = MODE_FADE;
}

static void fade_control(uint32_t delta_ms)
{
    if (fading.repeat == 0) {
        return;
    }

    fading.elapsed += delta_ms;
    if (fading.elapsed > fading.steps[fading.curr_step].duration) {
        fading.elapsed = 0;
        fading.color = fading.steps[fading.curr_step].to;
        fading.curr_step++;
        if (fading.curr_step == fading.step_num) {
            fading.curr_step = 0;
            fading.steps[0].from = fading.color;
            if (fading.repeat > 0) {
                fading.repeat--;
            }
        }
        return;
    }
    
    uint32_t color = lerp(fading.steps[fading.curr_step].from,
                          fading.steps[fading.curr_step].to,
                          fading.elapsed,
                          fading.steps[fading.curr_step].duration);

    fading.color = color;
}

static void fade_render()
{
    uint32_t color = apply_level(fading.color, aic_cfg->light.level_active);

    if (aic_cfg->light.rgb) {
        for (int i = 0; i < RGB_NUM; i++) {
            rgb_buf[i] = color << 8;
        }
    } else {
        memset(rgb_buf, 0, sizeof(rgb_buf));
    }

    uint16_t gray = (fading.color >> 16) | (fading.color >> 8) | fading.color;
    gray = apply_gray_level(gray, aic_cfg->light.level_active);
    if (aic_cfg->light.led) {
        for (int i = 0; i < LED_NUM; i++) {
            led_buf[i] = gray;
        }
    } else {
        memset(led_buf, 0, sizeof(led_buf));
    }
}

static void fade_update(uint32_t delta_ms)
{
    if (light_mode != MODE_FADE) {
        fading.color = 0x000000;
        fading.repeat = 0;
        return;
    }

    fade_control(delta_ms);
    fade_render();
}

static struct {
    struct {
        int current;
        int from;
        int to;
    } speed;
    struct {
        int current;
        int from;
        int to;
    } level;
    int smooth_ms;
    int elapsed;
} rainbow = { { 1, 1, 1 }, { 255, 255, 255 }, 0, 0 };

void light_rainbow(int8_t speed, uint32_t smooth_ms, uint8_t level)
{
    if (smooth_ms != 0) {
        rainbow.speed.from = rainbow.speed.current;
        rainbow.speed.to = speed;

        rainbow.level.from = rainbow.level.current;
        rainbow.level.to = level;

        rainbow.smooth_ms = smooth_ms;
        rainbow.elapsed = 0;
    } else {
        rainbow.speed.current = speed;
        rainbow.level.current = level;
        rainbow.smooth_ms = 0;
        rainbow.elapsed = 0;
    }
    
    light_mode = MODE_RAINBOW;
}

static int fast_sqrt(int x)
{
    int left = 0;
    int right = x;
    int result = 0;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int64_t sq = (int64_t)mid * mid;

        if (sq <= x) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return result;
}

static void rainbow_control(uint32_t delta_ms)
{
    if ((rainbow.smooth_ms == 0) || (rainbow.elapsed == rainbow.smooth_ms)) {
        return;
    }

    rainbow.elapsed += delta_ms;
    if (rainbow.elapsed > rainbow.smooth_ms) {
        rainbow.elapsed = rainbow.smooth_ms;
    }

    /* non linear speed change for better visual */
    int range = rainbow.speed.to - rainbow.speed.from;
    int progress = fast_sqrt(rainbow.elapsed * 10000 / rainbow.smooth_ms);
    rainbow.speed.current = rainbow.speed.from + range * progress / 100;

    range = rainbow.level.to - rainbow.level.from;
    progress = rainbow.elapsed * 100 / rainbow.smooth_ms;
    rainbow.level.current = rainbow.level.from + range * progress / 100;
}

#define RAINBOW_PITCH 37

static void rainbow_render()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 33333) { // no faster than 30Hz
        return;
    }
    last = now;

    static uint32_t rotator = 0;
    rotator = (rotator + rainbow.speed.current) % COLOR_WHEEL_SIZE;

    if (aic_cfg->light.rgb) {
        for (int i = 0; i < RGB_NUM; i++) {
            uint32_t index = (rotator + RAINBOW_PITCH * i) % COLOR_WHEEL_SIZE;
            rgb_buf[i] = apply_level(color_wheel[index], rainbow.level.current) << 8;
        }
    } else {
        memset(rgb_buf, 0, sizeof(rgb_buf));
    }

    if (aic_cfg->light.led) {
        for (int i = 0; i < LED_NUM; i++) {
            uint32_t index = (rotator + RAINBOW_PITCH * i) % COLOR_WHEEL_SIZE;
            led_buf[i] = apply_gray_level(gray_wheel[index], rainbow.level.current);
        }
    } else {
        memset(led_buf, 0, sizeof(led_buf));
    }
}

static void rainbow_update(uint32_t delta_ms)
{
    if (light_mode != MODE_RAINBOW) {
        rainbow.smooth_ms = 0;
        rainbow.speed.current = rainbow.speed.to;
        rainbow.level.current = rainbow.level.to;
        return;
    }
    rainbow_control(delta_ms);
    rainbow_render();
}

static void drive_led()
{
    dma_channel_configure(rgb_dma, &rgb_dma_cfg,
                          &pio0_hw->txf[0],
                          rgb_buf,
                          RGB_NUM,
                          true);
                        
    for (int i = 0; i < LED_NUM; i++) {
        pwm_set_gpio_level(led_gpio[i], led_buf[i]);
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
        gpio_set_drive_strength(RGB_PIN, GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_function(led_gpio[i], GPIO_FUNC_PWM);

        int slice = pwm_gpio_to_slice_num(led_gpio[i]);

        pwm_config cfg = pwm_get_default_config();
        pwm_config_set_clkdiv(&cfg, 4.f);
        pwm_init(slice, &cfg, true);
    }

    rgb_dma = dma_claim_unused_channel(true);
    rgb_dma_cfg = dma_channel_get_default_config(rgb_dma);
    channel_config_set_transfer_data_size(&rgb_dma_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&rgb_dma_cfg, true);
    channel_config_set_dreq(&rgb_dma_cfg, DREQ_PIO0_TX0);

    generate_color_wheel();
}

void light_update()
{
    static uint64_t last_time = 0;
    uint64_t now = time_us_64();
    if (now - last_time < 4000) { // no faster than 250Hz
        return;
    }
    uint32_t delta_ms = (now - last_time) / 1000;
    last_time = now;

    fade_update(delta_ms);
    rainbow_update(delta_ms);

    drive_led();
}
