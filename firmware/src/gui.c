
/*
 * GUI on a Touch Display (ST7789 + CST816)
 * WHowe <github.com/whowechina>
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "save.h"
#include "cli.h"
#include "nfc.h"

#include "st7789.h"
#include "cst816t.h"

#include "conthrax.h"
#include "upheaval.h"
#include "ltsaeada.h"
#include "light.h"
#include "gui.h"

void gui_init()
{
    cst816t_init_i2c(i2c1, 3, 2);
    cst816t_init(i2c1, 5, 4);
    cst816t_crop(10, 230, 35, 250, 240, 280);
    st7789_init_spi(spi1, 10, 11, 9);
    st7789_init(spi1, 8, 7, 0);
    st7789_crop(0, 20, 240, 280, true);
}

void gui_level(uint8_t level)
{
    st7789_dimmer(255 - level);
}

static void gui_numpad()
{
    static struct {
        uint16_t x;
        uint16_t y;
        char c;
    } signs[] = {
        {24, 26, '7'},
        {104, 26, '8'},
        {184, 26, '9'},
        {24, 96, '4'},
        {104, 96, '5'},
        {184, 96, '6'},
        {24, 166, '1'},
        {104, 166, '2'},
        {184, 166, '3'},
        {24, 236, '0'},
        {104, 236, ':'},
        {184, 236, ';'},
    };

    uint32_t color = rgb32_from_hsv(time_us_32() / 100000, 200, 250);
    for (int i = 0; i < 12; i++) {
        st7789_char(signs[i].x + 2, signs[i].y + 2, signs[i].c, &lv_conthrax, st7789_rgb565(0x101010));
        st7789_char(signs[i].x, signs[i].y, signs[i].c, &lv_conthrax, st7789_rgb565(color));
    }
}

static void status_title(int x, int y, const char *title, uint16_t color)
{
    st7789_text(x + 1, y + 1, title, &lv_lts16, 0x0000, ALIGN_CENTER);
    st7789_text(x, y, title, &lv_lts16, color, ALIGN_CENTER);
}

static void gui_status()
{
    st7789_spacing(1, 0);

    char buf[32];
    status_title(120, 3, "Serial Number", st7789_rgb565(0x00c000));
    sprintf(buf, "%016llx", board_id_64());
    st7789_text(120, 22, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 46, "Firmware Timestamp", st7789_rgb565(0x00c000));
    st7789_text(120, 66, built_time, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 89, "NFC Module", st7789_rgb565(0x00c000));
    sprintf(buf, "%s (%s)", nfc_module_name(), nfc_module_version());
    st7789_text(120, 105, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 132, "Light", st7789_rgb565(0x00c000));
    if (aic_cfg->light.rgb) {
        sprintf(buf, "RGB: %d ~ %d", aic_cfg->light.level_idle, aic_cfg->light.level_active);
    } else {
        sprintf(buf, "RGB: OFF");
    }
    st7789_text(120, 148, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 175, "LCD", st7789_rgb565(0x00c000));
    sprintf(buf, "Backlight: %d", aic_cfg->lcd.backlight);
    st7789_text(120, 191, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 218, "Reader", st7789_rgb565(0x00c000));
    int len = sprintf(buf, "Virtual AIC: %s\nMode: %s",
                      aic_cfg->reader.virtual_aic ? "ON" : "OFF",
                      mode_name(aic_cfg->reader.mode));
    if (aic_cfg->reader.mode == MODE_AUTO) {
        sprintf(buf + len, " (%s)", mode_name(aic_runtime.mode));
    }
    st7789_text(120, 234, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);
}

static void gui_credits()
{
    const char *credits =
        SET_COLOR(\x80\xff\x80) "AIC Pico (AIC Touch)\n"
        SET_COLOR(\x80\x80\xff) "https://github.com/whowechina\n\n\n"
        SET_COLOR(\x80\xff\x80) "THANKS TO\n\n"
        SET_COLOR(\x80\x80\x80) "CrazyRedMachine\n"
        "Sucareto    Bottersnike\n"
        "Gyt4    chujohiroto\n\n"
        "KiCAD    OnShape    Fritzing\n"
        "LVGL    .NET IoT\n"
        "JLCPCB    Raspberry\n\n"
        SET_COLOR(\x90\x90\x90) "and more...";

    st7789_text(120, 30, credits, &lv_lts14, st7789_rgb565(0xc0c060), ALIGN_CENTER);
}

static void run_background()
{
    st7789_scroll(0, 0);
    static uint16_t phase = 0;
    phase++;

    if (1) {

        for (int i = 0; i < 240; i++) {
            uint16_t color = st7789_rgb565(rgb32_from_hsv(phase + i, 255, 64));
            st7789_vline(i, 0, 280, color, 0xff);
        }
    }

    if (0) {
        uint16_t patt[14];
        for (int i = 0; i < 14; i++) {
            patt[i] = ((i + phase) % 7) ? 0x4040 : 0x0000;
        }
        st7789_fill(patt, 14, false);
    }

    if (0) {
        st7789_clear(0x0002, true);
    }
}


typedef struct {
    void (*render)();
    bool (*proc)(cst816t_report_t touch);
} gui_page_t;

static gui_page_t pages[] = {
    {gui_numpad, NULL},
    {gui_status, NULL},
    {gui_credits, NULL},
};

#define PAGE_NUM (sizeof(pages) / sizeof(pages[0]))

static int curr_page = 0;

typedef enum {
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN,
    HIT_LEFT,
    HIT_RIGHT,
} slide_dir_t;

static struct {
    bool sliding;
    slide_dir_t dir;
    int phase;
    int prev_page;
    const uint8_t *curve;
    size_t curve_len;
} slide;

static void start_slide(slide_dir_t dir, int new_page, const uint8_t *curve, size_t curve_len)
{
    slide.dir = dir;
    slide.prev_page = curr_page;
    slide.sliding = true;
    slide.phase = 0;
    curr_page = new_page;
    slide.curve = curve;
    slide.curve_len = curve_len;
}

static uint8_t scroll_curve[] = {
    1, 4, 9, 16, 25, 37, 51, 67, 85, 105, 127, 152, 168, 182,
    194, 204, 212, 219, 224, 228, 231, 233, 235, 237, 238, 239, 240,
};

static uint8_t spring_curve[] = {
    1, 3, 6, 10, 15, 21, 28, 36,
    28, 21, 15, 10, 6, 3, 1,
};

static void default_proc(cst816t_report_t touch)
{
    if (touch.gesture == GESTURE_SLIDE_LEFT) {
        if (curr_page > 0) {
            start_slide(SLIDE_LEFT, curr_page - 1, scroll_curve, sizeof(scroll_curve));
        } else {
            start_slide(HIT_LEFT, curr_page, spring_curve, sizeof(spring_curve));
        }
    } else if (touch.gesture == GESTURE_SLIDE_RIGHT) {
        if (curr_page < PAGE_NUM - 1) {
            start_slide(SLIDE_RIGHT, curr_page + 1, scroll_curve, sizeof(scroll_curve));
        } else {
            start_slide(HIT_RIGHT, curr_page, spring_curve, sizeof(spring_curve));
        }
    }
}

static void event_proc()
{
    cst816t_report_t touch = cst816t_read();
    if (!touch.updated) {
        return;
    }

    if ((curr_page > 0) && pages[curr_page].proc) {
        if (pages[curr_page].proc(touch)) {
            return;
        }
    }

    default_proc(touch);
}

static void sliding_render()
{
    if (!slide.sliding) {
        return;
    }


    int split = slide.curve[slide.phase];

    switch (slide.dir) {
        case SLIDE_LEFT:
            st7789_scroll(-split, 0);
            pages[slide.prev_page].render();
            st7789_scroll(240 - split, 0);
            pages[curr_page].render();
            break;
        case SLIDE_RIGHT:
            st7789_scroll(split, 0);
            pages[slide.prev_page].render();
            st7789_scroll(split - 240, 0);
            pages[curr_page].render();
            break;
        case SLIDE_UP:
            st7789_scroll(0, -split);
            pages[slide.prev_page].render();
            st7789_scroll(0, 280 - split);
            pages[curr_page].render();
            break;
        case SLIDE_DOWN:
            st7789_scroll(0, split);
            pages[slide.prev_page].render();
            st7789_scroll(0, split - 280);
            pages[curr_page].render();
            break;
        case HIT_LEFT:
            st7789_scroll(-split, 0);
            pages[curr_page].render();
            break;
        case HIT_RIGHT:
            st7789_scroll(split, 0);
            pages[curr_page].render();
            break;
    }

    slide.phase++;
    if (slide.phase >= slide.curve_len) {
        slide.sliding = false;
        return;
    }
}

void gui_loop()
{
    event_proc();

    run_background();

    if (slide.sliding) {
        sliding_render();
    } else {
        pages[curr_page].render();
    }

    st7789_render(true);
}
