/* Run Length Encoding (RLE) Codec
 * WHowe <github.com/whowechina>
 */

#include <stdint.h>
#include <stdlib.h>

#include "rle.h"

void rle_init(rle_t *rle, const void *input, size_t size)
{
    rle->src = input;
    rle->size = size;
    rle->pos = 0;
    rle->counter = 0;
}

void rle_x_init(rle_t *rle, const void *input, size_t size, uint32_t x)
{
    rle->x = x;
    rle_init(rle, input, size);
}

bool rle_eof(rle_t *rle)
{
    return (!rle->counter) && (rle->pos >= rle->size);
}

#define RLE_GET_TEMPLATE(type, condition) \
    if (rle->counter) { \
        rle->counter--; \
    } else if (rle->pos < rle->size) { \
        rle->value = ((const type *)rle->src)[rle->pos++]; \
        if (condition) { \
            rle->counter = ((const type *)rle->src)[rle->pos++]; \
        } \
    } \
    return rle->value;

#define RLE_GET(type) RLE_GET_TEMPLATE(type, true)
#define RLE_GET_X(type) RLE_GET_TEMPLATE(type, rle->value == rle->x)

uint8_t rle_get_uint8(rle_t *rle)
{
    RLE_GET(uint8_t);
}

uint16_t rle_get_uint16(rle_t *rle)
{
    RLE_GET(uint16_t);
}

uint8_t rle_x_get_uint8(rle_t *rle)
{
    RLE_GET_X(uint8_t);
}

uint16_t rle_x_get_uint16(rle_t *rle)
{
    RLE_GET_X(uint16_t);
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
