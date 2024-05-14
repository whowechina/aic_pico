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


void cst816t_init_i2c(i2c_inst_t *i2c, uint8_t scl, uint8_t sda)
{
    i2c_port = i2c;
    i2c_init(i2c_port, 200 * 000);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_set_function(sda, GPIO_FUNC_I2C);
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
    uint8_t xa;
    uint8_t xb;
    uint8_t ya;
    uint8_t yb;
    int8_t dx;
    int8_t dy;
} ctx = { 1, 1, 1, 1 };

void cst816t_set_ratio(uint8_t xa, uint8_t xb, uint8_t ya, uint8_t yb)
{
    ctx.xa = xa ? xa : 1;
    ctx.xb = xb;
    ctx.ya = ya ? ya : 1;
    ctx.yb = yb;
}

void cst816t_set_offset(int8_t dx, int8_t dy)
{
    ctx.dx = dx;
    ctx.dy = dy;
}

static void cst816t_read_reg_n(uint8_t reg, uint8_t *buf, uint8_t len)
{
    i2c_write_blocking_until(i2c_port, CST816T_I2C_ADDR, &reg, 1, true,
                             make_timeout_time_ms(1));
    i2c_read_blocking_until(i2c_port, CST816T_I2C_ADDR, buf, len, false,
                            make_timeout_time_ms(1));
}

cst816t_report_t cst816t_read()
{
    uint8_t buf[6];
    cst816t_read_reg_n(0x01, buf, 6);

    cst816t_report_t report = {
        .gesture = buf[0],
        .finger = buf[1],
        .event = (buf[2] >> 4) & 0x0f,
        .x = (((buf[2] & 0x0f) << 8) | buf[3]) * ctx.xb / ctx.xa + ctx.dx,
        .y = ((buf[4] << 8) | buf[5]) * ctx.yb / ctx.ya + ctx.dy,
    };

    return report;
}
