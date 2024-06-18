#include <stdint.h>

#include "st7789.h"
#include "rle.h"
#include "gfx.h"

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

static uint16_t pallete[2][16];

const uint16_t *gfx_anima_pallete(anima_pallete_t name)
{
    if (name == PALLETE_GRAYSCALE) {
        if (pallete[0][15] == 0) {
            for (int i = 0; i < 16; i++) {
                int scale = i * 31 / 15;
                pallete[0][i] = scale << 11 | scale << 5 | scale;
            }
        }
        return pallete[0];
    } else if (name == PALLETE_LIGHTNING) {
        if (pallete[1][15] == 0) {
            for (int i = 0; i < 8; i++) {
                int scale = i * 31 / 8;
                pallete[1][i] = scale;
            }
            for (int i = 0; i < 8; i++) {
                int scale = i * 31 / 7;
                pallete[1][i + 8] = scale << 11 | scale << 5 | 0xff;
            }
        }
        return pallete[1];
    }

    return NULL;
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
            st7789_pixel(x + i, y + j, color, value, 4);
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
            uint32_t mix = 0xff;
            uint32_t mixbits = 8;
            if (img->alpha.input) {
                mix = rle_get(&alpha_rle);
                mixbits = img->alpha.bits;
            }
        
            if (img->pallete) {
                pixel = img->pallete[pixel];
                if (!img->alpha.input) {
                    mix = pixel >> 16;
                }
            }

            st7789_pixel(x + j, y + i, pixel, mix, mixbits);
        }
    }
}


void gfx_char_draw(int x, int y, char c, const lv_font_t *font, uint16_t color)
{
    if (c < font->range_start || c >= font->range_start + font->range_length) {
        return;
    }

    const lv_font_dsc_t *dsc = font->dsc + c - font->range_start;
    const uint8_t *bitmap = font->bitmap + dsc->bitmap_index;

    uint8_t bpp = font->bit_per_pixel;
    uint8_t mask = (1L << bpp) - 1;
    uint8_t off_y = font->line_height - font->base_line - dsc->box_h - dsc->ofs_y;

    int crop_w = st7789_get_crop_width();
    int crop_h = st7789_get_crop_height();

    for (int i = 0; i < dsc->box_h; i++) {
        int dot_y = y + off_y + i;
        if ((dot_y < 0) || (dot_y > crop_h)) {
            continue;
        }
        for (int j = 0; j < dsc->box_w; j++) {
            int dot_x = x + dsc->ofs_x + j;
            if ((dot_x < 0) || (dot_x > crop_w)) {
                break;
            }
            uint16_t bits = (i * dsc->box_w + j) * bpp;
            uint8_t mix = (bitmap[bits / 8] >> ((8 - bpp) - (bits % 8))) & mask;
            st7789_pixel(dot_x, dot_y, color, mix, bpp);
        }
    }
}

static int spacing_x = 1, spacing_y = 1;

void gfx_text_spacing(int dx, int dy)
{
    spacing_x = dx;
    spacing_y = dy;
}

static uint16_t text_width(const char *text, const lv_font_t *font)
{
    uint16_t width = 0;
    for (; *text && (*text != '\n'); text++) {
        if (*text == '\x01') {
            text += 3;
            continue;
        } else if (*text == '\x02' || *text == '\x03') {
            continue;
        }
        if (*text - font->range_start < font->range_length) {
            width += (font->dsc[*text - font->range_start].adv_w >> 4) + spacing_x;
        }
    }
    return width;
}

void gfx_text_draw(int x, int y, const char *text,
                 const lv_font_t *font, uint16_t color, alignment_t align)
{
    uint16_t old_color = color;
    uint16_t curr_color = color;
    bool newline = true;
    int pos_x = x;
    for (; *text; text++) {
        if (*text == '\x01') { // set color
            old_color = curr_color;
            curr_color = st7789_rgb565(st7789_rgb32(text[1], text[2], text[3]));
            text += 3;
            continue;
        } else if (*text == '\x02') { // back to previous color
            uint16_t tmp = curr_color;
            curr_color = old_color;
            old_color = tmp;
            continue;
        } else if (*text == '\x03') { // reset to default color
            old_color = curr_color;
            curr_color = color;
            continue;
        } else if (*text == '\n') { // line wrap
            newline = true;
            pos_x = x;;
            y += font->line_height + spacing_y;
            continue;
        }
        if (newline) {
            int width = text_width(text, font);
            if (align == ALIGN_CENTER) {
                pos_x -= width / 2;
            } else if (align == ALIGN_RIGHT) {
                pos_x -= width;
            }
            newline = false;
        }
        gfx_char_draw(pos_x, y, *text, font, curr_color);
        pos_x += (font->dsc[*text - font->range_start].adv_w >> 4) + spacing_x;
    }
}
