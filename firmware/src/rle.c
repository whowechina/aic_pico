/* Run Length Encoding (RLE) Codec
 * WHowe <github.com/whowechina>
 */

#include <stdint.h>
#include <stdlib.h>

#include "rle.h"

void rle_init(rle_decoder_t *rle, const rle_src_t *src)
{
    rle->src = *src;
    rle->pos = 0;
    rle->counter = 0;
}

bool rle_eof(rle_decoder_t *rle)
{
    return (!rle->remaining) && (!rle->counter) && (rle->pos >= rle->src.size);
}

#define RLE_GET_TEMPLATE(type) \
    if (rle->src.encoding == RLE_NONE) { \
        return ((const type *)rle->src.input)[rle->pos++]; \
    } \
    if (rle->counter) { \
        rle->counter--; \
    } else if (rle->pos < rle->src.size) { \
        rle->value = ((const type *)rle->src.input)[rle->pos++]; \
        if ((rle->src.encoding == RLE_RLE) || (rle->value == rle->src.x)) { \
            rle->counter = ((const type *)rle->src.input)[rle->pos++]; \
        } \
    }

uint8_t rle_get_uint8(rle_decoder_t *rle)
{
    RLE_GET_TEMPLATE(uint8_t);
    return rle->value;
}

uint16_t rle_get_uint16(rle_decoder_t *rle)
{
    RLE_GET_TEMPLATE(uint16_t);
    return rle->value;
}

uint8_t rle_get_uint4(rle_decoder_t *rle)
{
    if (rle->remaining) {
        rle->remaining = false;
        return rle->value & 0x0f;
    }

    RLE_GET_TEMPLATE(uint8_t);
    rle->remaining = true;
    return rle->value >> 4;
}

uint32_t rle_get(rle_decoder_t *rle)
{
    if (rle->src.bits == 4) {
        return rle_get_uint4(rle);
    } else if (rle->src.bits == 8) {
        return rle_get_uint8(rle);
    } else {
        return rle_get_uint16(rle);
    }
}

#define PUSH_DATA(data) \
    output[pos++] = data;

#define BEGIN_COUNT \
    value = input[i]; \
    PUSH_DATA(value); \
    num = 0; \
    counting = true;

#define END_COUNT \
    PUSH_DATA(num); \
    counting = false;

#define RLE_ENCODE_TEMPLATE(type, max, condition) \
    size_t pos = 0; \
    bool counting = false; \
    uint32_t value; \
    uint32_t num; \
    for (size_t i = 0; i < size; i++) { \
        if (!counting) { \
            if (condition) { \
                BEGIN_COUNT; \
            } else { \
                PUSH_DATA(input[i]); \
            } \
            continue; \
        } \
        if (input[i] == value) { \
            num++; \
            if (num >= max) { \
                END_COUNT; \
            } \
            continue; \
        } \
        if (counting) { \
            END_COUNT; \
            if (condition) { \
                BEGIN_COUNT; \
            } else { \
                PUSH_DATA(input[i]); \
            } \
        } \
    } \
    if (counting) { \
        END_COUNT; \
    } \
    return pos;

#define RLE_ENCODE(type, max) RLE_ENCODE_TEMPLATE(type, max, true)
#define RLE_X_ENCODE(type, max, x) RLE_ENCODE_TEMPLATE(type, max, input[i] == x)

size_t rle_encode_uint8(uint8_t *output, const uint8_t *input, size_t size)
{
    RLE_ENCODE(uint8_t, UINT8_MAX)
}

size_t rle_encode_uint16(uint16_t *output, const uint16_t *input, size_t size)
{
    RLE_ENCODE(uint16_t, UINT16_MAX)
}

size_t rle_x_encode_uint8(uint8_t *output, const uint8_t *input, size_t size, uint32_t x)
{
    RLE_X_ENCODE(uint8_t, UINT8_MAX, x)
}

size_t rle_x_encode_uint16(uint16_t *output, const uint16_t *input, size_t size, uint32_t x)
{
    RLE_X_ENCODE(uint16_t, UINT16_MAX, x)
}
