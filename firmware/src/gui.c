
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
        st7789_vline(i, 0, 320,
                    st7789_rgb565(rgb32_from_hsv(phase % 256 + i, 255, 128)), 0xff);
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

static void run_status()
{
    char buf[32];

    st7789_text(5, 5, "SN:", &lv_upheaval, 1, st7789_rgb565(0x00ff00));
    sprintf(buf, "%016llx", board_id_64());
    st7789_text(5, 25, buf, &lv_upheaval, 1, st7789_rgb565(0x808080));

    st7789_text(5, 45, "Built:", &lv_upheaval, 1, st7789_rgb565(0x00ff00));
    st7789_text(5, 65, built_time, &lv_upheaval, 1, st7789_rgb565(0x808080));

    st7789_text(5, 85, "NFC Module:", &lv_upheaval, 1, st7789_rgb565(0x00ff00));
    sprintf(buf, "%s (%s)\n", nfc_module_name(), nfc_module_version());
    st7789_text(5, 105, buf, &lv_upheaval, 1, st7789_rgb565(0x808080));
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
