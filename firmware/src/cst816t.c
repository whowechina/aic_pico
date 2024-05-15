/*
 * CST816T Touch Sensor Driver
 * WHowe <github.com/whowechina>
 * 
 */

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

void cst816t_init(i2c_inst_t *i2c, uint8_t trst, uint8_t tint)
{
    i2c_port = i2c;

    gpio_init(trst);
    gpio_set_dir(trst, GPIO_OUT);
    gpio_put(trst, 1);

    gpio_init(tint);
    gpio_set_dir(tint, GPIO_IN);
    gpio_pull_up(tint);
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

cst816t_report_t cst816t_read()
{
    uint8_t buf[6];

    cst816t_read_reg_n(0x01, buf, 6);

    cst816t_report_t report = {
        .gesture = buf[0],
        .finger = buf[1],
        .event = (buf[2] >> 4) & 0x0f,
        .raw_x = ((buf[2] & 0x0f) << 8) | buf[3],
        .raw_y = ((buf[4] & 0x0f) << 8) | buf[5],
    };

    int x = (report.raw_x - ctx.x1) * ctx.width / (ctx.x2 - ctx.x1);
    int y = (report.raw_y - ctx.y1) * ctx.height / (ctx.y2 - ctx.y1);
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

    report.x = x;
    report.y = y;

    return report;
}
