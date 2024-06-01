/*
 * ST7789 Buffered Display Driver for Pico with DMA Support
 * WHowe <github.com/whowechina>
 * 
 * LEDK is driven by PWM to adjust brightness
 */

#include <stdint.h>
#include <stdbool.h>

#include "hardware/spi.h"

#include "lvgl_font.h"

void st7789_reset();
void st7789_init_spi(spi_inst_t *port, uint8_t sck, uint8_t tx, uint8_t csn);
void st7789_init(spi_inst_t *port, uint8_t dc, uint8_t rst, uint8_t ledk);
void st7789_crop(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool absolute);
void st7789_dimmer(uint8_t level);
void st7789_vsync();
void st7789_render(bool vsync);

static inline uint32_t st7789_rgb32(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) | (g << 8) | b;
}

static inline uint16_t st7789_rgb565(uint32_t rgb32)
{
    return ((rgb32 >> 8) & 0xf800) | ((rgb32 >> 5) & 0x07e0) | ((rgb32 >> 3) & 0x001f);
}

void st7789_clear(uint16_t color, bool raw);
void st7789_fill(uint16_t *pattern, size_t size, bool raw);
uint16_t *st7789_vram(uint16_t x, uint16_t y);
void st7789_vramcpy(uint32_t offset, const void *src, size_t count);
void st7789_pixel(int x, int y, uint16_t color, uint8_t mix);
void st7789_hline(int x, int y, uint16_t w, uint16_t color, uint8_t mix);
void st7789_vline(int x, int y, uint16_t h, uint16_t color, uint8_t mix);
void st7789_bar(int x, int y, uint16_t w, uint16_t h, uint16_t color, uint8_t mix);
void st7789_line(int x0, int y0, int x1, int y1, uint16_t color, uint8_t mix);

void st7789_scroll(int dx, int dy);

/* char and text out only supports 1/2/4/8 bit-per-pixel */
void st7789_char(int x, int y, char c, const lv_font_t *font, uint16_t color);

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} alignment_t;

void st7789_spacing(int dx, int dy);

/* Color escape scheme:
  "\x01\xRR\xGG\xBB" set color;
  "\x02" back to previous color;
  "\x03" reset to default color;
 */

#define SET_COLOR(rgb) "\x01" #rgb
#define PREV_COLOR "\x02"
#define RESET_COLOR "\x03"

void st7789_text(int x, int y, const char *text, const lv_font_t *font,
                 uint16_t color, alignment_t align);
