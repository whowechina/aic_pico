
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
#include "light.h"

#include "aime.h"
#include "bana.h"

#include "st7789.h"
#include "cst816t.h"

#include "res/resource.h"
#include "gfx.h"
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

static struct {
    nfc_card_name card;
    uint64_t time;
} card_splash;

static inline bool card_splash_active()
{
    return time_us_64() - card_splash.time < 3000000;
}

void gui_report_card(nfc_card_name card)
{
    card_splash.card = card;
    card_splash.time = time_us_64();
}

static int tapped_key = -1;

static void draw_home_keypad()
{
    static uint8_t glow_frame[12] = { 0 };
    const char *signs_text = "7894561230:;";

    uint32_t color = rgb32_from_hsv(time_us_32() / 100000, 200, 250);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int key = row * 3 + col;
            int x = col * 80 + 24;
            int y = row * 70 + 26;
            if (key == tapped_key) {
                glow_frame[key] = 1;
            }
            if ((glow_frame[key] > 0) && (glow_frame[key] < anima_glow.frames)) {
                int tail = anima_glow.frames - glow_frame[key];
                uint16_t glow_color = 0xffff;
                if (tail < 6) {
                    glow_color = st7789_gray(tail * 0x30);
                }
                gfx_anima_mix(&anima_glow, x - 18, y - 20, glow_frame[key], glow_color);
                glow_frame[key]++;
            }
            char c = signs_text[row * 3 + col];
            gfx_char_draw(x + 2, y + 2, c, &lv_conthrax, st7789_rgb565(0x101010));
            gfx_char_draw(x, y, c, &lv_conthrax, st7789_rgb565(color));
        }
    }
}

static void center_image(const image_t *img)
{
    gfx_img_draw(120 - img->width / 2, 140 - img->height / 2, img);
}

static void draw_home_aime()
{
    center_image(&image_aime_reader);
}

static void draw_home_bana()
{
    center_image(&image_bana_reader);
}

static void draw_home_card()
{
    if (card_splash.card == CARD_AIC_SEGA) {
        center_image(&image_aic_sega);
    } else if (card_splash.card == CARD_AIC_KONAMI){
        center_image(&image_aic_konami);
    } else if (card_splash.card == CARD_AIC_BANA) {
        center_image(&image_aic_bana);
    } else if (card_splash.card == CARD_AIC_NESICA) {
        center_image(&image_aic_nesica);
    } else if (card_splash.card == CARD_AIC) {
        center_image(&image_aic_generic);
    } else if (card_splash.card == CARD_MIFARE) {
        center_image(&image_mifare);
    } else if (card_splash.card == CARD_AIME) {
        center_image(&image_aime);
    } else if (card_splash.card == CARD_BANA) {
        center_image(&image_bana);
    } else if (card_splash.card == CARD_NESICA) {
        center_image(&image_nesica);
    } else if (card_splash.card == CARD_VICINITY) {
        center_image(&image_vicinity);
    } else if (card_splash.card == CARD_EAMUSE) {
        center_image(&image_eamuse);
    }
}

static void draw_home()
{
    if (card_splash_active()) {
        draw_home_card();
    } else if (aime_is_active()) {
        draw_home_aime();
    } else if (bana_is_active()) {
        draw_home_bana();
    } else {
        draw_home_keypad();
    }
}

uint16_t gui_keypad_read()
{
    static int last_tapped = -1;
    static uint64_t last_active;
    uint64_t now = time_us_32();
    const uint8_t map[] = { 6, 7, 8, 3, 4, 5, 0, 1, 2, 9, 10, 11 };

    if ((last_tapped >= 0) && (now - last_active < 100000)) {
        return 1 << map[last_tapped];
    }
    
    if (tapped_key >= 0) {
        last_tapped = tapped_key;
        last_active = now;
        return 1 << map[tapped_key];
    }

    return 0;
}

static bool proc_home(cst816t_report_t touch)
{
    if (aime_is_active() || bana_is_active()) {
        return false;
    }

    switch (touch.gesture) {
        case GESTURE_NONE:
            tapped_key = -1;
            break;
        case GESTURE_TAP:
            tapped_key = touch.y / 70 * 3 + touch.x / 80;
            break;
        default:
            return false;
    }

    return true;
}

static void status_title(int x, int y, const char *title, uint16_t color)
{
    gfx_text_draw(x + 1, y + 1, title, &lv_lts16, 0x0000, ALIGN_CENTER);
    gfx_text_draw(x, y, title, &lv_lts16, color, ALIGN_CENTER);
}

static void draw_status()
{
    gfx_text_spacing(1, 0);

    char buf[32];
    status_title(120, 3, "Serial Number", st7789_rgb565(0x00c000));
    sprintf(buf, "%016llx", board_id_64());
    gfx_text_draw(120, 22, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 46, "Firmware Timestamp", st7789_rgb565(0x00c000));
    gfx_text_draw(120, 66, built_time, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 89, "NFC Module", st7789_rgb565(0x00c000));
    sprintf(buf, "%s (%s)", nfc_module_name(), nfc_module_version());
    gfx_text_draw(120, 105, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 132, "Light", st7789_rgb565(0x00c000));
    if (aic_cfg->light.rgb) {
        sprintf(buf, "RGB: %d ~ %d", aic_cfg->light.level_idle, aic_cfg->light.level_active);
    } else {
        sprintf(buf, "RGB: OFF");
    }
    gfx_text_draw(120, 148, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 175, "LCD", st7789_rgb565(0x00c000));
    sprintf(buf, "Backlight: %d", aic_cfg->lcd.backlight);
    gfx_text_draw(120, 191, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 218, "Reader", st7789_rgb565(0x00c000));
    int len = sprintf(buf, "Virtual AIC: %s\nMode: %s",
                      aic_cfg->reader.virtual_aic ? "ON" : "OFF",
                      mode_name(aic_cfg->reader.mode));
    if (aic_cfg->reader.mode == MODE_AUTO) {
        sprintf(buf + len, " (%s)", mode_name(aic_runtime.mode));
    }
    gfx_text_draw(120, 234, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);
}

static void draw_credits()
{
    const char *credits =
        SET_COLOR(\x80\xff\x80) "AIC Pico (AIC Touch)\n"
        SET_COLOR(\x80\x80\xff) "https://github.com/whowechina\n\n\n"
        SET_COLOR(\x80\xff\x80) "THANKS TO\n\n"
        SET_COLOR(\xb0\xb0\xb0) "CrazyRedMachine\n"
        "Sucareto    Bottersnike\n"
        "Gyt4    chujohiroto\n\n"
        "KiCAD    OnShape    Fritzing\n"
        "LVGL    .NET IoT\n"
        "JLCPCB    Raspberry\n\n"
        SET_COLOR(\x90\x90\x90) "and more...";

    gfx_text_draw(120, 30, credits, &lv_lts14, st7789_rgb565(0xc0c060), ALIGN_CENTER);
}

static void gen_pallete(uint16_t pallete[16], uint32_t color)
{
    uint32_t r = (color >> 16) & 0xff;
    uint32_t g = (color >> 8) & 0xff;
    uint32_t b = color & 0xff;

    for (int i = 0; i < 16; i++) {
        uint32_t mix = st7789_rgb32(r * i / 30, g * i / 30, b * i / 30);
        pallete[i] = st7789_rgb565(mix);
    }
}

static void run_background()
{
    static int phase = 0;
    phase++;

    uint16_t pallete[16];

    if (card_splash_active()) {
        gen_pallete(pallete, 0x0000ff);
        gfx_anima_draw(&anima_light, 0, 0, phase, gfx_anima_pallete(PALLETE_LIGHTNING));
    } else {
        uint32_t color = rgb32_from_hsv(time_us_32() / 100000 + 128, 200, 250);
        gen_pallete(pallete, color);
        gfx_anima_draw(&anima_star, 0, 0, phase, pallete);
    }
}

typedef struct {
    void (*render)();
    bool (*proc)(cst816t_report_t touch);
} gui_page_t;

static gui_page_t pages[] = {
    {draw_home, proc_home},
    {draw_status, NULL},
    {draw_credits, NULL},
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

    if ((curr_page >= 0) && pages[curr_page].proc) {
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
    run_background();

    if (slide.sliding) {
        sliding_render();
    } else {
        pages[curr_page].render();
    }

    st7789_flush(false);

    /* Control things when updating LCD */
    gui_level(aic_cfg->lcd.backlight);
    event_proc();

    st7789_vsync();
}
