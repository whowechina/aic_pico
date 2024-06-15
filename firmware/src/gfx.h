#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "rle.h"

typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t frames;
    uint32_t size;
    const uint32_t *index;
    const uint8_t *data;
} anima_t;

typedef struct {
    uint16_t width;
    uint16_t height;

    rle_src_t pixels;
    const uint32_t *pallete; // always 256 colors, RGB565 (bits 0..15) + alpha (bits 16..23)
    rle_src_t alpha; // ignored when pallete exists
} image_t;

extern const uint16_t white_pallete[16];

void gfx_anima_draw(const anima_t *ani, int x, int y, int frame, const uint16_t pallete[16]);
void gfx_anima_mix(const anima_t *ani, int x, int y, int frame, uint16_t color);

void gfx_img_draw(int x, int y, const image_t *img);

#endif
