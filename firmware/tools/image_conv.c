/* Image compression tool for AIC Pico
 * WHowe <github.com/whowechina>
 * RLE or RLE X can be used.
 * ENCODE and ENCODE_X are for compression.
 * ENCODE_PAL converts a 256-color pallete) from 32bit RGBA to RGB565A8.
 *
 * Transparent PNG is converted by https://lvgl.io/tools/imageconverter,
 *   Use indexed 8bit or 4bit.
 *   Even if the image is in 4bit, you can still use RLE 8bit compression.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../src/rle.c"
#include "suica_220.c"

uint16_t buf[2*1024*1024];
uint8_t *output8 = (uint8_t *)buf;
uint16_t *output16 = (uint16_t *)buf;

void test_rle_x()
{
    uint8_t input[] = { 1, 1, 1, 2, 3, 3, 3, 3, 4, 4, 5, 7, 0};

    printf("Input: ");
    for (int i = 0; i < sizeof(input); i++) {
        printf("%d ", input[i]);
    }
    printf("\n");

    int size = rle_x_encode_uint8(output8, input, sizeof(input), 1);
    for (int i = 0; i < size; i++) {
        printf("%d ", output8[i]);
    }
    printf("\nDecode:");
    rle_decoder_t rle;
    rle_init(&rle, &(rle_src_t){ output8, size, RLE_RLE_X, 1 });
    for (int i = 0; i < sizeof(input); i++) {
        printf("%d ", rle_get_uint8(&rle));
    }
    printf("\n");

    size = rle_x_encode_uint8(output8, input, sizeof(input), 3);
    for (int i = 0; i < size; i++) {
        printf("%d ", output8[i]);
    }
    printf("\nDecode:");
    rle_init(&rle, &(rle_src_t){ output8, size, RLE_RLE_X, 3 });
    for (int i = 0; i < sizeof(input); i++) {
        printf("%d ", rle_get_uint8(&rle));
    }
    printf("\n");

    size = rle_x_encode_uint8(output8, input, sizeof(input), 4);
    for (int i = 0; i < size; i++) {
        printf("%d ", output8[i]);
    }
    printf("\nDecode:");
    rle_init(&rle, &(rle_src_t){ output8, size, RLE_RLE_X, 4 });
    for (int i = 0; i < sizeof(input); i++) {
        printf("%d ", rle_get_uint8(&rle));
    }
    printf("\n");

    size = rle_x_encode_uint8(output8, input, sizeof(input), 0);
    for (int i = 0; i < size; i++) {
        printf("%d ", output8[i]);
    }
    printf("\nDecode:");
    rle_init(&rle, &(rle_src_t){ output8, size, RLE_RLE_X, 0 });
    for (int i = 0; i < sizeof(input); i++) {
        printf("%d ", rle_get_uint8(&rle));
    }
    printf("\n");

    size = rle_x_encode_uint8(output8, input, sizeof(input), 8);
    for (int i = 0; i < size; i++) {
        printf("%d ", output8[i]);
    }
    printf("\nDecode:");
    rle_init(&rle, &(rle_src_t){ output8, size, RLE_RLE_X, 8 });
    for (int i = 0; i < sizeof(input); i++) {
        printf("%d ", rle_get_uint8(&rle));
    }
    printf("\n");
}

void print_array(size_t size)
{
    for (int i = 0; i < size; i++) {
        if (i % 16 == 0) {
            printf("\n   ");
        }
        printf(" 0x%02x,", output8[i]);
    }
    printf("\n");
}

/* Work only for arrays */
#define DO_ENCODE(bits, input, size, name) \
    do { \
        size_t len = rle_encode_uint##bits(output##bits, (const uint##bits##_t *)input, size); \
        printf("const uint8_t %s[] = {\n", #name); \
        printf("    // %s (%zd -> RLE %zd)", #input, size * bits / 8, len * bits / 8); \
        print_array(len * bits / 8); \
        printf("};\n\n"); \
    } while (0);

/* Work only for arrays */
#define DO_ENCODE_X(bits, input, x, size, name) \
    do { \
        size_t len = rle_x_encode_uint##bits(output##bits, (const uint##bits##_t *)input, size, x); \
        printf("const uint8_t %s[] = {\n", #name); \
        printf("    // %s (%zd -> RLE(%x) %zd)", #input, size * bits / 8, x, len * bits / 8); \
        print_array(len * bits / 8); \
        printf("};\n\n"); \
    } while (0);

#define ENCODE(bits, input) DO_ENCODE(bits, input, sizeof(input), input)
#define ENCODE_X(bits, input, x) DO_ENCODE_X(bits, input, x, sizeof(input), input)
#define ENCODE_ALPHA(alpha) \
    to_4bpp(alpha, sizeof(alpha)); \
    DO_ENCODE(8, buf, sizeof(alpha) / 2, alpha);
#define ENCODE_X_ALPHA(alpha, x) \
    to_4bpp(alpha, sizeof(alpha)); \
    DO_ENCODE_X(8, buf, x, sizeof(alpha) / 2, alpha);

void encode_pallette(const uint8_t *pallete, size_t size)
{
    for (int i = 0; i < size / 4; i++) {
        if (i % 4 == 0) {
            printf("\n   ");
        }
        const uint8_t *color = pallete + i * 4;
        uint8_t r = color[0];
        uint8_t g = color[1];
        uint8_t b = color[2];
        uint8_t a = color[3];
        uint16_t rgb565 = (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
        
        printf(" 0x%06x,", (a << 16) | rgb565);
    }
}

void to_4bpp(const uint8_t *alpha, size_t size)
{
    for (int i = 0; i < size / 2; i++) {
        buf[i] = (alpha[i * 2] << 8) | alpha[i * 2 + 1];
    }
}

#define ENCODE_PAL(pallete) \
    printf("const uint32_t %s[] = {\n", #pallete); \
    printf("    // %s Pallete (RGB565A8*%ld)", #pallete, sizeof(pallete) / 4); \
    encode_pallette(pallete, sizeof(pallete)); \
    printf("\n};\n\n");

int main()
{
    ENCODE(8, image_suica_pixels);
    ENCODE_PAL(image_suica_pal);
    return 0;
}
