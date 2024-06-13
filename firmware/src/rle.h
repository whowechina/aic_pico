/* Run Length Encoding (RLE) Codec
 * WHowe <github.com/whowechina>
 *
 * RLE is regular, RLE_X only encodes a special value
 */

#ifndef RLE_H
#define RLE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const void *src;
    size_t size;
    int pos;
    uint32_t x;
    uint32_t value;
    uint32_t counter;
} rle_t;

void rle_init(rle_t *rle, const void *input, size_t size);
void rle_x_init(rle_t *rle, const void *input, size_t size, uint32_t x);

bool rle_eof(rle_t *rle);

uint8_t rle_get_uint8(rle_t *rle);
uint16_t rle_get_uint16(rle_t *rle);

uint8_t rle_x_get_uint8(rle_t *rle);
uint16_t rle_x_get_uint16(rle_t *rle);

/* No protection, make sure output is large enough */
size_t rle_encode_uint8(uint8_t *output, const uint8_t *input, size_t size);
size_t rle_encode_uint16(uint16_t *output, const uint16_t *input, size_t size);

size_t rle_x_encode_uint8(uint8_t *output, const uint8_t *input, size_t size, uint32_t x);
size_t rle_x_encode_uint16(uint16_t *output, const uint16_t *input, size_t size, uint32_t x);

#endif
