#ifndef ANIMA_H
#define ANIMA_H

#include <stdint.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t frames;
    const uint32_t *index;
    const uint8_t *data;
} anima_t;

extern const uint16_t white_pallete[16];

void anima_draw(const anima_t *ani, int x, int y, int frame, const uint16_t pallete[16]);
void anima_mix(const anima_t *ani, int x, int y, int frame, uint16_t color);

#endif
