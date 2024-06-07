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
    bool updated;
    bool touched;
    uint16_t x;
    uint16_t y;
    uint16_t raw_x;
    uint16_t raw_y;
} cst816t_raw_t;

typedef enum {
    GESTURE_NONE = 0,
    GESTURE_TAP,
    GESTURE_SLIDE_UP,
    GESTURE_SLIDE_DOWN,
    GESTURE_SLIDE_LEFT,
    GESTURE_SLIDE_RIGHT,
} gesture_t;

typedef struct {
    bool updated;
    bool touched;
    gesture_t gesture;
    uint16_t x;
    uint16_t y;
    uint16_t touch_x;
    uint16_t touch_y;
    uint16_t release_x;
    uint16_t release_y;
    uint16_t raw_x;
    uint16_t raw_y;
} cst816t_report_t;

void cst816t_update();
cst816t_raw_t cst816t_read_raw();
cst816t_report_t cst816t_read();
