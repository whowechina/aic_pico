/*
 * ST7789 Buffered Display Driver for Pico with DMA Support
 * WHowe <github.com/whowechina>
 * 
 * LEDK is driven by PWM to adjust brightness
 */

#include <stdint.h>
#include <stdbool.h>

#include "hardware/spi.h"

void st7789_reset();
void st7789_init_spi(spi_inst_t *port, uint8_t sck, uint8_t tx, uint8_t csn);
void st7789_init(spi_inst_t *port, uint8_t dc, uint8_t rst, uint8_t ledk);
void st7789_crop(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool absolute);
uint16_t st7789_get_crop_width();
uint16_t st7789_get_crop_height();
void st7789_dimmer(uint8_t level);
void st7789_vsync();
void st7789_flush(bool vsync);

#define st7789_rgb32(r, g, b) ((r << 16) | (g << 8) | b)
#define st7789_rgb565(rgb32) ((rgb32 >> 8) & 0xf800) | ((rgb32 >> 5) & 0x0780) | ((rgb32 >> 3) & 0x001f)
#define st7789_gray(value) ((value >> 3 << 11) | (value >> 3 << 6) | (value >> 3))  

void st7789_clear(uint16_t color, bool raw);
void st7789_fill(uint16_t *pattern, size_t size, bool raw);
uint16_t *st7789_vram(uint16_t x, uint16_t y);
void st7789_vramcpy(uint32_t offset, const void *src, size_t count);
void st7789_pixel_raw(int x, int y, uint16_t color);
void st7789_pixel(int x, int y, uint16_t color, uint8_t mix, uint8_t bits);
void st7789_hline(int x, int y, uint16_t w, uint16_t color, uint8_t mix);
void st7789_vline(int x, int y, uint16_t h, uint16_t color, uint8_t mix);
void st7789_bar(int x, int y, uint16_t w, uint16_t h, uint16_t color, uint8_t mix);
void st7789_line(int x0, int y0, int x1, int y1, uint16_t color, uint8_t mix);

void st7789_scroll(int dx, int dy);
