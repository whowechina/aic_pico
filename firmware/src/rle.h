/* Run Length Encoding (RLE) Codec
 * WHowe <github.com/whowechina>
 *
 * RLE is regular, RLE_X only encodes a special value
 */

#ifndef RLE_H
#define RLE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RLE_NONE,
    RLE_RLE,
    RLE_RLE_X
} rle_encoding_t;

typedef struct {
    const uint8_t *input;
    rle_encoding_t encoding;
    size_t bits;
    size_t size;
    uint32_t x;
} rle_src_t;

typedef struct {
    rle_src_t src;
    int pos;
    uint32_t value;
    uint32_t counter;
    bool remaining;
} rle_decoder_t;

void rle_init(rle_decoder_t *rle, const rle_src_t *src);

bool rle_eof(rle_decoder_t *rle);

uint8_t rle_get_uint8(rle_decoder_t *rle);
uint16_t rle_get_uint16(rle_decoder_t *rle);
uint8_t rle_get_uint4(rle_decoder_t *rle);
uint32_t rle_get(rle_decoder_t *rle);

/* No protection, make sure output is large enough */
size_t rle_encode_uint8(uint8_t *output, const uint8_t *input, size_t size);
size_t rle_encode_uint16(uint16_t *output, const uint16_t *input, size_t size);

size_t rle_x_encode_uint8(uint8_t *output, const uint8_t *input, size_t size, uint32_t x);
size_t rle_x_encode_uint16(uint16_t *output, const uint16_t *input, size_t size, uint32_t x);

#endif
