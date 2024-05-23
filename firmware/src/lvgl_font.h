/* 
 * Simplified LVGL Font Structure
 * Modified from LVGL project: https://lvgl.io/
 */

#ifndef LV_FONT_H
#define LV_FONT_H

#include <stdint.h>

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

#endif
