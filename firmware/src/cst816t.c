/*
 * CST816T Touch Sensor Driver
 * WHowe <github.com/whowechina>
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "cst816t.h"

#define CST816T_I2C_ADDR 0x15

static i2c_inst_t *i2c_port = i2c0;

static void cst816t_read_reg_n(uint8_t reg, uint8_t *buf, uint8_t len)
{
    i2c_write_blocking_until(i2c_port, CST816T_I2C_ADDR, &reg, 1, true,
                             make_timeout_time_ms(1));
    i2c_read_blocking_until(i2c_port, CST816T_I2C_ADDR, buf, len, false,
                            make_timeout_time_ms(1));
}

void cst816t_init_i2c(i2c_inst_t *i2c, uint8_t scl, uint8_t sda)
{
    i2c_port = i2c;
    i2c_init(i2c_port, 200 * 1000);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_pull_up(scl);
    gpio_pull_up(sda);
}

static void irq_handler(uint gpio, uint32_t events)
{
    cst816t_update();
}

void cst816t_init(i2c_inst_t *i2c, uint8_t trst, uint8_t tint)
{
    i2c_port = i2c;

    gpio_init(trst);
    gpio_set_dir(trst, GPIO_OUT);
    gpio_put(trst, 1);

    gpio_init(tint);
    gpio_set_dir(tint, GPIO_IN);
    gpio_pull_up(tint);

    gpio_set_irq_enabled_with_callback(tint, GPIO_IRQ_EDGE_FALL, true, &irq_handler);
}

static struct {
    uint16_t x1;
    uint16_t x2;
    uint16_t y1;
    uint16_t y2;
    uint16_t width;
    uint16_t height;
} ctx = { 1, 1, 1, 1 };

void cst816t_crop(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t width, uint16_t height)
{
    ctx.x1 = x1;
    ctx.x2 = x2;
    ctx.y1 = y1;
    ctx.y2 = y2;
    ctx.width = width;
    ctx.height = height;
}

static struct {
    bool touched;
    uint16_t x;
    uint16_t y;
} reading;

void cst816t_update()
{
    uint8_t buf[6];
    cst816t_read_reg_n(0x01, buf, 6);
    reading.touched = buf[1];
    reading.x = ((buf[2] & 0x0f) << 8) | buf[3];
    reading.y = ((buf[4] & 0x0f) << 8) | buf[5];
}

cst816t_raw_t cst816t_read_raw()
{
    static cst816t_raw_t old = { 0 };
    cst816t_raw_t raw = {
        .updated = false,
        .touched = reading.touched,
        .x = old.x,
        .y = old.y,
        .raw_x = reading.x,
        .raw_y = reading.y,
    };

    if ((old.raw_x == raw.raw_x) && (old.raw_y == raw.raw_y) &&
        (old.touched == raw.touched)) {
        return raw;
    }

    int x = (raw.raw_x - ctx.x1) * ctx.width / (ctx.x2 - ctx.x1);
    int y = (raw.raw_y - ctx.y1) * ctx.height / (ctx.y2 - ctx.y1);
    if (x < 0) {
        x = 0;
    } else if (x >= ctx.width) {
        x = ctx.width - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= ctx.height) {
        y = ctx.height - 1;
    }

    raw.x = x;
    raw.y = y;

    raw.updated = true;
    old = raw;

    return raw;
}

cst816t_report_t cst816t_read()
{
    cst816t_raw_t raw = cst816t_read_raw();

    static struct {
        cst816t_report_t old_report;
        bool x_revert;
        bool x_slide;
        bool y_revert;
        bool y_slide;
    } ctx;

    cst816t_report_t report = ctx.old_report;
    report.updated = false;
    if (report.gesture != GESTURE_NONE) {
        report.gesture = GESTURE_NONE;
        report.updated = true;
    }

    if (raw.updated) {
        /* touch related changes */
        report.x = raw.x;
        report.y = raw.y;
        report.touched = raw.touched;
        if (!ctx.old_report.touched && report.touched) {
            report.touch_x = report.x;
            report.touch_y = report.y;
        } else if (ctx.old_report.touched && !report.touched) {
            report.release_x = report.x;
            report.release_y = report.y;
            int dx = report.release_x - report.touch_x;
            int dy = report.release_y - report.touch_y;
            if ((abs(dx) < 10) && (abs(dy) < 10)) {
                report.gesture = GESTURE_TAP;
            } else if (dx > 30) {
                report.gesture = GESTURE_SLIDE_RIGHT;
            } else if (dx < -30) {
                report.gesture = GESTURE_SLIDE_LEFT;
            }
        }
        report.updated = true;
    }

    if (0) {
        /* timing */
        report.updated = true;
    }

    ctx.old_report = report;

    return report;
}
