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
void st7789_crop(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void st7789_dimmer(uint8_t level);
void st7789_vsync();
void st7789_render(bool vsync);

uint16_t st7789_rgb565(uint32_t rgb32);
void st7789_clear();
void st7789_pixel(uint16_t x, uint16_t y, uint16_t color);
void st7789_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void st7789_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void st7789_bar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7789_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
