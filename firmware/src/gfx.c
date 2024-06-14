#include <stdint.h>

#include "st7789.h"
#include "rle.h"
#include "gfx.h"

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

void gfx_anima_draw(const anima_t *ani, int x, int y, int frame, const uint16_t pallete[16])
{
    uint16_t width = ani->width;
    uint16_t height = ani->height;
    frame = frame % ani->frames;

    rle_t rle;
    rle_x_init(&rle, ani->data + ani->index[frame], ani->size, 0);

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width / 2; i++) {
            uint8_t value = rle_x_get_uint8(&rle);
            st7789_pixel_raw(x + i * 2, y + j, pallete[value >> 4]);
            st7789_pixel_raw(x + i * 2 + 1, y + j, pallete[value & 0x0f]);
        }
    }
}

void gfx_anima_mix(const anima_t *ani, int x, int y, int frame, uint16_t color)
{
    uint16_t width = ani->width;
    uint16_t height = ani->height;
    frame = frame % ani->frames;

    rle_t rle;
    rle_x_init(&rle, ani->data + ani->index[frame], ani->size, 0);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width / 2; i++) {
            uint8_t value = rle_x_get_uint8(&rle);
            st7789_pixel(x + i * 2, y + j, color, value >> 4 << 4);
            st7789_pixel(x + i * 2 + 1, y + j, color, (value & 0x0f) << 4);
        }
    }
}

static rle_t pixels_rle;
static rle_t alpha_rle;
static int pixels_pos;
static int alpha_pos;

#define INIT_RLE(which) \
    if (img->which##_rle_x) { \
        rle_x_init(&which##_rle, img->which, img->width * img->height, img->which##_x); \
    } else if (img->which##_rle) { \
        rle_init(&which##_rle, img->which, img->width * img->height); \
    } else { \
        which##_pos = 0; \
    }

#define GET_DATA(which, type) \
    (img->which##_rle_x ? \
        rle_x_get_##type(&which##_rle) : \
        img->which##_rle ? \
            rle_get_##type(&which##_rle) :\
            img->which[which##_pos++])

void gfx_img_draw(int x, int y, const image_t *img)
{
    INIT_RLE(pixels);
    INIT_RLE(alpha);

    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            uint16_t pixel = GET_DATA(pixels, uint16);
            uint8_t mix = img->alpha ? GET_DATA(alpha, uint8) : 0xff;
            st7789_pixel(x + j, y + i, pixel, mix);
        }
    }
}
