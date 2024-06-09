#include <stdint.h>
#include "st7789.h"
#include "anima.h"

const uint16_t white_pallete[16] = {
    st7789_gray(0x00),
    st7789_gray(0x10),
    st7789_gray(0x20),
    st7789_gray(0x30),
    st7789_gray(0x40),
    st7789_gray(0x50),
    st7789_gray(0x60),
    st7789_gray(0x70),
    st7789_gray(0x80),
    st7789_gray(0x90),
    st7789_gray(0xa0),
    st7789_gray(0xb0),
    st7789_gray(0xc0),
    st7789_gray(0xd0),
    st7789_gray(0xe0),
    st7789_gray(0xf0),
};

static const uint8_t *compressed_data;
static int zero_count = 0;
static inline void decode_start(const uint8_t *data)
{
    compressed_data = data;
    zero_count = 0;
}

static inline uint8_t decode_byte()
{
    if (zero_count) {
        zero_count--;
        return 0;
    }

    uint8_t value = *compressed_data;
    compressed_data++;

    if (value == 0) {
        zero_count = *compressed_data - 1;
        compressed_data++;
        return 0;
    }

    return value;
}

void anima_draw(const anima_t *ani, int x, int y, int frame, const uint16_t pallete[16])
{
    uint16_t width = ani->width;
    uint16_t height = ani->height;
    frame = frame % ani->frames;

    decode_start(ani->data + ani->index[frame]);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width / 2; i++) {
            uint8_t value = decode_byte();
            st7789_pixel_raw(x + i * 2, y + j, pallete[value >> 4]);
            st7789_pixel_raw(x + i * 2 + 1, y + j, pallete[value & 0x0f]);
        }
    }
}

void anima_mix(const anima_t *ani, int x, int y, int frame, uint16_t color)
{
    uint16_t width = ani->width;
    uint16_t height = ani->height;
    frame = frame % ani->frames;

    decode_start(ani->data + ani->index[frame]);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width / 2; i++) {
            uint8_t value = decode_byte();
            st7789_pixel(x + i * 2, y + j, color, value >> 4 << 4);
            st7789_pixel(x + i * 2 + 1, y + j, color, (value & 0x0f) << 4);
        }
    }
}