/*
 * Image compression tool
 * From 8bit grayscale to 4bit grayscale and zero-run-length encoding
 * Input: img_data[], img_width, img_height
 * Frame count is calculated from img_data size and frame size (img_width * img_height)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../src/bg2.h"

static int col = 0;

static void line_begin()
{
    col = 0;
}

static void out_byte(uint8_t byte)
{
    if (col == 0) {
        printf("\n   ");
    }
    printf(" 0x%02x,", byte);
    col = (col + 1) % 16;
}

static int zero_count = 0;

static int out_zero()
{
    if (!zero_count) {
        return 0;
    }

    out_byte(0);
    out_byte(zero_count);
    zero_count = 0;

    return 2;
}

static int encode_byte(uint8_t byte)
{
    if (byte == 0) {
        zero_count++;
    }

    int written = 0;
    
    if ((byte != 0) || (zero_count == 255)) {
        written += out_zero();
    }

    if (byte != 0) {
        out_byte(byte);
        written++;
    }

    return written;
}

static int encode_finish()
{
    return out_zero();
}

static inline uint8_t gs8bto4b(const uint8_t bytes[2])
{
    return (bytes[0] & 0xf0) | (bytes[1] >> 4);
}

static int out_frame(int frame, int frame_size)
{
    const uint8_t *frame_data = img_data + frame * frame_size;
    printf("\n    // frame %d", frame);
    line_begin();
    int written = 0;
    for (int i = 0; i < frame_size / 2; i++) {
        written += encode_byte(gs8bto4b(frame_data + i * 2));
    }
    written += encode_finish();
    return written;
}

const char *headlines =
    "#include <stdint.h>\n\n"
    "typedef struct {\n"
    "    uint16_t width;\n"
    "    uint16_t height;\n"
    "    uint32_t frames;\n"
    "    const uint32_t *index;\n"
    "    const uint8_t *data;\n"
    "} img_frames_t;\n\n";

static const int frame_size = img_width * img_height;
static const int frame_count = sizeof(img_data) / frame_size;
int main()
{
    int frame_index[frame_count];

    printf("%s", headlines);
    printf("const uint8_t img_data[] = {");
    int sum = 0;
    for (int frame = 0; frame < frame_count; frame++) {
        frame_index[frame] = sum;
        sum += out_frame(frame, frame_size);
    }
    printf("\n};\n\n");

    printf("const uint32_t img_index[] = {\n   ");
    for (int frame = 0; frame < frame_count; frame++) {
        printf(" %d,", frame_index[frame]);
    }
    printf("\n};\n\n");

    printf("const img_frames_t images = {\n");
    printf("    .width = %d,\n", img_width);
    printf("    .height = %d,\n", img_height);
    printf("    .frames = %d,\n", frame_count);
    printf("    .index = img_index,\n");
    printf("    .data = img_data,\n");
    printf("};\n");

    fprintf(stderr, "Frame count: %d\n", frame_count);
    fprintf(stderr, "Total: %d\n", sum);
}