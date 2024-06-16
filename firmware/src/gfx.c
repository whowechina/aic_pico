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
    rle_decoder_t rle;
    rle_init(&rle, 
             &(rle_src_t){ ani->data + ani->index[frame % ani->frames], 
                           RLE_RLE_X, 4, ani->size, 0x00 }
            );
                            
    for (int j = 0; j < ani->height; j++) {
        for (int i = 0; i < ani->width; i++) {
            uint8_t value = rle_get_uint4(&rle);
            st7789_pixel_raw(x + i, y + j, pallete[value]);
        }
    }
}

void gfx_anima_mix(const anima_t *ani, int x, int y, int frame, uint16_t color)
{
    rle_decoder_t rle;
    rle_init(&rle, 
             &(rle_src_t){ ani->data + ani->index[frame % ani->frames], 
                           RLE_RLE_X, 4, ani->size, 0x00 }
            );

    for (int j = 0; j < ani->height; j++) {
        for (int i = 0; i < ani->width; i++) {
            uint8_t value = rle_get_uint4(&rle);
            st7789_pixel(x + i, y + j, color, value);
        }
    }
}

void gfx_img_draw(int x, int y, const image_t *img)
{
    rle_decoder_t pixels_rle;
    rle_decoder_t alpha_rle;

    rle_init(&pixels_rle, &img->pixels);

    if (img->alpha.input) {
        rle_init(&alpha_rle, &img->alpha);
    }

    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            uint32_t pixel = rle_get(&pixels_rle);
            uint32_t mix = img->alpha.input ? rle_get(&alpha_rle) : 0xff;
        
            if (img->pallete) {
                pixel = img->pallete[pixel];
                if (!img->alpha.input) {
                    mix = pixel >> 16;
                }
            }

            st7789_pixel(x + j, y + i, pixel, mix);
        }
    }
}
