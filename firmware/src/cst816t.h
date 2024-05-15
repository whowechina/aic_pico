/*
 * CST816T Touch Sensor Driver
 * WHowe <github.com/whowechina>
 * 
 */

#include <stdint.h>
#include <stdbool.h>

#include "hardware/i2c.h"

void cst816t_init_i2c(i2c_inst_t *i2c, uint8_t scl, uint8_t sda);
void cst816t_init(i2c_inst_t *i2c, uint8_t trst, uint8_t tint);

void cst816t_crop(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t width, uint16_t height);

typedef struct {
    uint8_t gesture;
    uint8_t finger;
    uint8_t event;
    uint16_t x;
    uint16_t y;
    uint16_t raw_x;
    uint16_t raw_y;
} cst816t_report_t;

cst816t_report_t cst816t_read();
