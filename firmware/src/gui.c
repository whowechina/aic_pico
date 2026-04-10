
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

#include "lis3dh.h"

void gui_init()
{
    cst816t_init_i2c(i2c1, 3, 2);
    cst816t_init(i2c1, 5, 4);
    cst816t_crop(10, 230, 35, 250, 240, 280);
    st7789_init_spi(spi1, 10, 11, 9);
    st7789_init(spi1, 8, 7, 0);
    st7789_crop(0, 20, 240, 280);
}

void gui_level(uint8_t level)
{
    st7789_dimmer(255 - level);
}

static struct {
    uint64_t time;
    nfc_card_name card;
    struct {
        uint8_t len;
        uint8_t octects[15];
    } real, virtual;
} card_splash;

static inline bool card_splash_active()
{
    return time_us_64() - card_splash.time < 3000000;
}

void gui_report_card_name(nfc_card_name card)
{
    card_splash.card = card;
    card_splash.time = time_us_64();
}

void gui_report_card_id(const uint8_t *id, int len, bool virtual)
{
    if (len > sizeof(card_splash.real.octects)) {
        len = sizeof(card_splash.real.octects);
    }

    if (virtual) {
        card_splash.virtual.len = len;
        memcpy(card_splash.virtual.octects, id, len);
        return;
    }
    
    if ((card_splash.real.len == len) && 
        (memcmp(card_splash.real.octects, id, len) == 0)) {
        return;
    }

    card_splash.real.len = len;
    memcpy(card_splash.real.octects, id, len);
    card_splash.virtual.len = 0;
}

static int tapped_key = -1;
static bool orient_up = true;

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

static void center_image(const image_t *img, int v_offset)
{
    gfx_img_draw(120 - img->width / 2, 140 + v_offset - img->height / 2, img);
}

static void draw_home_aime()
{
    center_image(&image_aime_reader, 0);
}

static void draw_home_bana()
{
    center_image(&image_bana_reader, 0);
}

static void draw_home_card()
{
    if (card_splash.card == CARD_AIC_SEGA) {
        center_image(&image_aic_sega, -24);
    } else if (card_splash.card == CARD_AIC_KONAMI){
        center_image(&image_aic_konami, -24);
    } else if (card_splash.card == CARD_AIC_BANA) {
        center_image(&image_aic_bana, -24);
    } else if (card_splash.card == CARD_AIC_NESICA) {
        center_image(&image_aic_nesica, -24);
    } else if (card_splash.card == CARD_AIC) {
        center_image(&image_aic_generic, -24);
    } else if (card_splash.card == CARD_SUICA) {
        center_image(&image_suica, -24);
    } else if (card_splash.card == CARD_MIFARE) {
        center_image(&image_mifare, -24);
    } else if (card_splash.card == CARD_AIME) {
        center_image(&image_aime, -24);
    } else if (card_splash.card == CARD_BANA) {
        center_image(&image_bana, -24);
    } else if (card_splash.card == CARD_NESICA) {
        center_image(&image_nesica, -24);
    } else if (card_splash.card == CARD_VICINITY) {
        center_image(&image_vicinity, -24);
    } else if (card_splash.card == CARD_EAMUSE) {
        center_image(&image_eamuse, -24);
    }
}

static void draw_card_id()
{
    char idstr[40];

    if (card_splash.real.len > 0) {
        int pos = 0;
        idstr[0] = '\0';
        for (int i = 0; i < card_splash.real.len; i++) {
            pos += sprintf(idstr + pos, " %02X", card_splash.real.octects[i]);
        }

        const lv_font_t *font = card_splash.real.len > 6 ? &lv_dejavu16 : &lv_dejavu18;
        const char *id_title = "UID";
        gfx_text_draw(120, 196, id_title, &lv_lts13, st7789_rgb565(0xb0b000), ALIGN_CENTER);
        gfx_text_draw(118, 210, idstr, font, st7789_rgb565(0xe0e000), ALIGN_CENTER);
    }

    if (card_splash.virtual.len > 0) {
        int pos = 0;
        idstr[0] = '\0';
        for (int i = 0; i < card_splash.virtual.len; i++) {
            pos += sprintf(idstr + pos, "%02X", card_splash.virtual.octects[i]);
        }
        const char *cardio_title = "CardIO";
        gfx_text_draw(120, 236, cardio_title, &lv_lts13, st7789_rgb565(0x00b0b0), ALIGN_CENTER);
        gfx_text_draw(120, 250, idstr, &lv_dejavu16, st7789_rgb565(0x00e0e0), ALIGN_CENTER);
    }
}

static void draw_home()
{
    if (card_splash_active()) {
        draw_home_card();
        draw_card_id();
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
            {
                int col = touch.x / 80 % 3;
                int row = touch.y / 70 % 4;
                
                if (!orient_up) {
                    col = 2 - col;
                    row = 3 - row;
                }
                tapped_key = row * 3 + col;
            }
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

    char buf[64];
    status_title(120, 3, "Serial Number", st7789_rgb565(0x00c000));
    sprintf(buf, "%016llx", board_id_64());
    gfx_text_draw(120, 22, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 46, "Firmware Timestamp", st7789_rgb565(0x00c000));
    gfx_text_draw(120, 66, built_time, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 89, "NFC Module", st7789_rgb565(0x00c000));
    sprintf(buf, "%s (%s)", nfc_module_name(), nfc_module_version());
    gfx_text_draw(120, 105, buf, &lv_lts18, st7789_rgb565(0xc0c0c0), ALIGN_CENTER);

    status_title(120, 132, "Light", st7789_rgb565(0x00c000));
    if (aic_cfg->light.rgb_en) {
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

static const int8_t sin_table[256] = {
     0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
    48, 51, 54, 57, 60, 62, 65, 68, 70, 73, 75, 78, 80, 83, 85, 87,
    90, 92, 94, 96, 98,100,102,104,106,107,109,111,112,114,115,117,
   118,119,121,122,123,124,125,126,126,127,127,127,127,127,127,127,
   127,127,127,127,127,127,126,126,125,124,123,122,121,119,118,117,
   115,114,112,111,109,107,106,104,102,100, 98, 96, 94, 92, 90, 87,
    85, 83, 80, 78, 75, 73, 70, 68, 65, 62, 60, 57, 54, 51, 48, 46,
    43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 12,  9,  6,  3,  0, -3,
    -6, -9,-12,-16,-19,-22,-25,-28,-31,-34,-37,-40,-43,-46,-48,-51,
   -54,-57,-60,-62,-65,-68,-70,-73,-75,-78,-80,-83,-85,-87,-90,-92,
   -94,-96,-98,-100,-102,-104,-106,-107,-109,-111,-112,-114,-115,-117,
  -118,-119,-121,-122,-123,-124,-125,-126,-126,-127,-127,-127,-127,-127,
  -127,-127,-127,-127,-127,-127,-127,-126,-126,-125,-124,-123,-122,-121,
  -119,-118,-117,-115,-114,-112,-111,-109,-107,-106,-104,-102,-100,-98,
   -96,-94,-92,-90,-87,-85,-83,-80,-78,-75,-73,-70,-68,-65,-62,-60,
   -57,-54,-51,-48,-46,-43,-40,-37,-34,-31,-28,-25,-22,-19,-16,-12,
    -9, -6, -3
};

#define CELL_SIZE 8
static void run_particle(uint16_t angle)
{
    enum {
        GRID_W = 240 / CELL_SIZE,
        GRID_H = 280 / CELL_SIZE,
        FP_BITS = 8,
        FP_ONE = 1 << FP_BITS,
        PARTICLE_COUNT = (GRID_W * GRID_H) * 75 / 100,
        GRAVITY_ACCEL = 30,
        PRESSURE_ACCEL = 20,
        JITTER_ACCEL = 6,
        MAX_SPEED = FP_ONE * 2,
    };

    typedef struct {
        int32_t x;
        int32_t y;
        int16_t vx;
        int16_t vy;
    } fluid_particle_t;

    static bool inited = false;
    static fluid_particle_t particles[PARTICLE_COUNT];
    static uint8_t density[GRID_H][GRID_W]; // each cell stores particle count in low-res tube space
    static uint32_t rng = 0x6c8e9cf5u;
    static int last_angle = 0;

    const int tube_x = (st7789_get_crop_width() - GRID_W * CELL_SIZE) / 2;
    const int tube_y = (st7789_get_crop_height() - GRID_H * CELL_SIZE) / 2;
    const int max_x = (GRID_W - 1) * FP_ONE;
    const int max_y = (GRID_H - 1) * FP_ONE;

    uint8_t hue = time_us_32() / 100000 + 127;
    uint16_t bg_color = 0;
    uint16_t water_color = st7789_rgb565(rgb32_from_hsv(hue, 255, 180));
    uint16_t foam_color = st7789_rgb565(rgb32_from_hsv(hue, 160, 160));

    if (!inited) {
        // Start with 30% fill from the bottom to make initial shape stable.
        for (int i = 0; i < PARTICLE_COUNT; i++) {
            int cell_x = i % GRID_W;
            int cell_y = GRID_H - 1 - (i / GRID_W);
            particles[i].x = cell_x * FP_ONE + (FP_ONE / 2);
            particles[i].y = cell_y * FP_ONE + (FP_ONE / 2);
            particles[i].vx = 0;
            particles[i].vy = 0;
        }
        last_angle = angle;
        inited = true;
    }

    memset(density, 0, sizeof(density));

    // 8-bit trig lookup based gravity vector, angle=0 means gravity points down.
    int angle_norm = angle % 360;
    if (angle_norm < 0) {
        angle_norm += 360;
    }
    int phase = (angle_norm * 256) / 360;
    int16_t gx = sin_table[phase & 0xff];
    int16_t gy = sin_table[(phase + 64) & 0xff];

    int dangle = angle - last_angle;
    if (dangle > 180) {
        dangle -= 360;
    } else if (dangle < -180) {
        dangle += 360;
    }
    last_angle = angle;

    // First pass: build local density field from particle positions.
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        int cx = particles[i].x >> FP_BITS;
        int cy = particles[i].y >> FP_BITS;

        if (cx < 0) cx = 0;
        if (cx >= GRID_W) cx = GRID_W - 1;
        if (cy < 0) cy = 0;
        if (cy >= GRID_H) cy = GRID_H - 1;

        if (density[cy][cx] < 255) {
            density[cy][cx]++;
        }
    }

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        fluid_particle_t *p = &particles[i];

        int cx = p->x >> FP_BITS;
        int cy = p->y >> FP_BITS;
        if (cx < 0) cx = 0;
        if (cx >= GRID_W) cx = GRID_W - 1;
        if (cy < 0) cy = 0;
        if (cy >= GRID_H) cy = GRID_H - 1;

        // Base gravity acceleration.
        p->vx += (gx * GRAVITY_ACCEL) >> 7;
        p->vy += (gy * GRAVITY_ACCEL) >> 7;

        // Pressure-like force from neighboring density gradient.
        int left = (cx > 0) ? density[cy][cx - 1] : density[cy][cx];
        int right = (cx < GRID_W - 1) ? density[cy][cx + 1] : density[cy][cx];
        int up = (cy > 0) ? density[cy - 1][cx] : density[cy][cx];
        int down = (cy < GRID_H - 1) ? density[cy + 1][cx] : density[cy][cx];
        p->vx += (left - right) * PRESSURE_ACCEL;
        p->vy += (up - down) * PRESSURE_ACCEL;

        // When angle changes, add a tangential impulse to make visible slosh.
        if (dangle != 0) {
            p->vx += ((-gy * dangle) >> 6);
            p->vy += ((gx * dangle) >> 6);
        }

        // Overcrowded cells get tiny deterministic jitter to break symmetry.
        int c = density[cy][cx];
        if (c > 2) {
            rng = rng * 1664525u + 1013904223u;
            p->vx += (int16_t)((int)((rng >> 24) & 0x03) - 1) * JITTER_ACCEL;
            rng = rng * 1664525u + 1013904223u;
            p->vy += (int16_t)((int)((rng >> 24) & 0x03) - 1) * JITTER_ACCEL;
        }

        if (p->vx > MAX_SPEED) p->vx = MAX_SPEED;
        if (p->vx < -MAX_SPEED) p->vx = -MAX_SPEED;
        if (p->vy > MAX_SPEED) p->vy = MAX_SPEED;
        if (p->vy < -MAX_SPEED) p->vy = -MAX_SPEED;

        int nx = p->x + p->vx;
        int ny = p->y + p->vy;

        if (nx < 0) {
            nx = 0;
            p->vx = -(p->vx * 3) / 4;
        } else if (nx > max_x) {
            nx = max_x;
            p->vx = -(p->vx * 3) / 4;
        }

        if (ny < 0) {
            ny = 0;
            p->vy = -(p->vy * 3) / 4;
        } else if (ny > max_y) {
            ny = max_y;
            p->vy = -(p->vy * 3) / 4;
        }

        p->x = nx;
        p->y = ny;

        // Viscous damping to keep movement smooth and stable.
        p->vx = (p->vx * 245) >> 8;
        p->vy = (p->vy * 245) >> 8;
    }

    memset(density, 0, sizeof(density));
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        int cx = particles[i].x >> FP_BITS;
        int cy = particles[i].y >> FP_BITS;

        if (cx < 0) cx = 0;
        if (cx >= GRID_W) cx = GRID_W - 1;
        if (cy < 0) cy = 0;
        if (cy >= GRID_H) cy = GRID_H - 1;

        if (density[cy][cx] < 255) {
            density[cy][cx]++;
        }
    }

    st7789_clear(bg_color, false);

    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            uint8_t n = density[y][x];
            if (n == 0) {
                continue;
            }
            uint16_t color = (n > 1) ? foam_color : water_color;
            uint8_t mix = (n >= 3) ? 0xff : (n == 2 ? 0xe0 : 0xc8);
            st7789_bar(tube_x + x * CELL_SIZE, tube_y + y * CELL_SIZE,
                       CELL_SIZE - 1, CELL_SIZE - 1, color, mix);
        }
    }
}

static void bg_deepspace()
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

#define ORIENT_BODY_PHASE_DEG 270
#define ORIENT_UP_ANGLE_DEG 0
#define ORIENT_SWITCH_WINDOW_DEG 45
#define ORIENT_NEAR_FLAT_XY_MIN_SQ (1000 * 1000)
#define ORIENT_NEAR_FLAT_XY_Z_RATIO 9

static int norm_deg(int deg)
{
    deg %= 360;
    if (deg < 0) {
        deg += 360;
    }
    return deg;
}

static int angle_sep(int a, int b)
{
    int d = abs(norm_deg(a) - norm_deg(b));
    return (d > 180) ? (360 - d) : d;
}

static void update_orientation(uint16_t angle)
{
    int x = lis3dh_read_x();
    int y = lis3dh_read_y();
    int z = lis3dh_read_z();

    int xy_sq = x * x + y * y;
    int zz = z * z;

    bool near_flat = (xy_sq < ORIENT_NEAR_FLAT_XY_MIN_SQ) ||
                     (xy_sq * ORIENT_NEAR_FLAT_XY_Z_RATIO < zz);

    int up_angle = norm_deg(ORIENT_UP_ANGLE_DEG);
    int down_angle = norm_deg(up_angle + 180);
    bool in_up_window = angle_sep(angle, up_angle) <= ORIENT_SWITCH_WINDOW_DEG;
    bool in_down_window = angle_sep(angle, down_angle) <= ORIENT_SWITCH_WINDOW_DEG;

    if (near_flat) {
        return;
    }

    if (in_down_window && orient_up) {
        orient_up = false;
    } else if (in_up_window && !orient_up) {
        orient_up = true;
    }
}

void gui_loop()
{
    st7789_scroll(0, 0);
    st7789_invert(false);

    if (lis3dh_is_present()) {
        uint16_t raw_angle = (uint16_t)(lis3dh_read() * 360 / 4096);
        uint16_t gravity_angle = norm_deg(raw_angle + ORIENT_BODY_PHASE_DEG);

        run_particle(gravity_angle);

        if (aic_cfg->lcd.orientation == 0) {
            update_orientation(gravity_angle);
        }
    } else {
        bg_deepspace();
    }

    if (aic_cfg->lcd.orientation == 1) {
        orient_up = true;
    } else if (aic_cfg->lcd.orientation == 2) {
        orient_up = false;
    }

    st7789_invert(!orient_up);
    if (slide.sliding) {
        sliding_render();
    } else {
        pages[curr_page].render();
    }

#ifdef PICO_RP2350
    st7789_vsync();
#endif

    st7789_flush();
    /* Control things when updating LCD */
    gui_level(aic_cfg->lcd.backlight);
    event_proc();

#ifndef PICO_RP2350
    st7789_vsync();
#endif
}
