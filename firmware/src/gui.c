
/*
 * GUI on a Touch Display (ST7789 + CST816)
 * WHowe <github.com/whowechina>
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

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

static void run_numpad()
{
    static uint16_t phase = 0;
    phase++;
    for (int i = 0; i < 240; i++) {
        uint16_t color = st7789_rgb565(rgb32_from_hsv(phase + i, 255, 128));
        st7789_vline(i, 0, 320, color, 0xff);
    }

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

static void run_status()
{
    uint16_t patt[14];
    for (int i = 0; i < 14; i++) {
        patt[i] = (i % 7) ? 0x4040 : 0x0000;
    }

    st7789_fill(patt, 14);

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

void gui_loop()
{
    uint8_t page = 1;

    switch (page) {
    case 0:
        run_numpad();
        break;
    case 1:
        run_status();
        break;
    }

    st7789_render(true);
}
