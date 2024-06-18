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
    rle_src_t alpha; // ignored when pallete exists, always 4bpp
} image_t;

void gfx_anima_draw(const anima_t *ani, int x, int y, int frame, const uint16_t pallete[16]);
void gfx_anima_mix(const anima_t *ani, int x, int y, int frame, uint16_t color);

void gfx_img_draw(int x, int y, const image_t *img);

typedef enum {
    PALLETE_GRAYSCALE,
    PALLETE_LIGHTNING,
} anima_pallete_t;

/* lightning, grayscale */
const uint16_t *gfx_anima_pallete(anima_pallete_t pallete);

/* LV Font is Simplified from LVGL (https://lvgl.io/) Font Structure */
typedef struct {
    uint32_t bitmap_index; /**< Start index of the bitmap. A font can be max 4 GB.*/
    uint32_t adv_w;        /**< Draw the next glyph after this width. 28.4 format (real_value * 16 is stored).*/
    uint16_t box_w;        /**< Width of the glyph's bounding box*/
    uint16_t box_h;        /**< Height of the glyph's bounding box*/
    int16_t ofs_x;         /**< x offset of the bounding box*/
    int16_t ofs_y;         /**< y offset of the bounding box. Measured from the top of the line*/
} lv_font_dsc_t;

typedef struct {
    uint8_t range_start;
    uint8_t range_length;
    uint8_t bit_per_pixel;
    uint16_t line_height;
    uint16_t base_line;
    const lv_font_dsc_t *dsc;
    const uint8_t *bitmap;
} lv_font_t;

/* char and text out only supports 1/2/4/8 bit-per-pixel */
void gfx_char_draw(int x, int y, char c, const lv_font_t *font, uint16_t color);

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} alignment_t;

void gfx_text_spacing(int dx, int dy);

/* Color escape scheme:
  "\x01\xRR\xGG\xBB" set color;
  "\x02" back to previous color;
  "\x03" reset to default color;
 */

#define SET_COLOR(rgb) "\x01" #rgb
#define PREV_COLOR "\x02"
#define RESET_COLOR "\x03"

void gfx_text_draw(int x, int y, const char *text, const lv_font_t *font,
                   uint16_t color, alignment_t align);

#endif
