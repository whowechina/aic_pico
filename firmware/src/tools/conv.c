/*
 * Image compression tool (zero-run-length encoding)
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bg_anima_8b.h"


static int col = 0;

static void line_begin()
{
    col = 0;
}

static void out_byte(uint8_t byte)
{
    if (col == 0) {
        printf("\n   ");
        col == 0;
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

static int encode_byte(uint8_t byte, bool end)
{
    if (byte == 0) {
        zero_count++;
    }

    int written = 0;
    
    if ((byte != 0) || (zero_count == 255) || end) {
        written += out_zero();
    }

    if (byte != 0) {
        out_byte(byte);
        written++;
    }

    return written;
}
    
static int out_frame(int frame, int frame_size)
{
    const uint8_t *frame_data = stars_gif.data + frame * frame_size;
    printf("\n    // frame %d", frame);
    line_begin();
    int written = 0;
    for (int i = 0; i < frame_size - 1; i++) {
        written += encode_byte(frame_data[i], false);
    }
    written += encode_byte(frame_data[frame_size - 1], true);
    return written;
}


int main()
{
    int frame_index[stars_gif.frames];

    printf("const uint8_t data[] = {");
    int sum = 0;
    for (int frame = 0; frame < stars_gif.frames; frame++) {
        frame_index[frame] = sum;
        sum += out_frame(frame, stars_gif.width * stars_gif.height);
    }
    printf("\n};\n");

    printf("const uint32_t index[] = {\n   ");
    for (int frame = 0; frame < stars_gif.frames; frame++) {
        printf(" %d,", frame_index[frame]);
    }
    printf("\n};\n");
    printf("Total: %d\n", sum);
}