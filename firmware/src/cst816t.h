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

void cst816t_set_ratio(uint8_t xa, uint8_t xb, uint8_t ya, uint8_t yb);
void cst816t_set_offset(int8_t dx, int8_t dy);

typedef struct {
    uint8_t gesture; 
    uint8_t finger;
    uint8_t event;
    uint16_t x;
    uint16_t y;
} cst816t_report_t;

cst816t_report_t cst816t_read();
