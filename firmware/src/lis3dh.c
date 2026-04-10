/*
 * LIS3DH 3-Axis Accelerometer
 * I2C Interface, AD0=High (addr 0x19)
 * WHowe <github.com/whowechina>
 */

#include "lis3dh.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "hardware/i2c.h"

#define LIS3DH_ADDR         0x19
#define LIS3DH_REG_WHO_AM_I 0x0F
#define LIS3DH_WHO_AM_I_ID  0x33
#define LIS3DH_REG_CTRL1    0x20
#define LIS3DH_REG_OUT_X_L  0x28  /* X_L..Z_H at 0x28..0x2D */

/* Set bit7 of register address to enable address auto-increment on LIS3DH */
#define LIS3DH_MULTI_READ(reg) ((reg) | 0x80)

static i2c_inst_t *lis3dh_i2c = i2c0;

static int lis3dh_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_write_blocking_until(lis3dh_i2c, LIS3DH_ADDR, buf, 2, false,
                                    make_timeout_time_ms(1));
}

static int lis3dh_read_bytes(uint8_t reg, uint8_t *data, int len)
{
    if (i2c_write_blocking_until(lis3dh_i2c, LIS3DH_ADDR, &reg, 1, true,
                                  make_timeout_time_ms(1)) != 1) {
        return -1;
    }
    return i2c_read_blocking_until(lis3dh_i2c, LIS3DH_ADDR, data, len, false,
                                    make_timeout_time_ms(len));
}

static uint8_t chip_id = 0;

void lis3dh_init(i2c_inst_t *i2c_port, uint8_t odr)
{
    lis3dh_i2c = i2c_port;

    if ((lis3dh_read_bytes(LIS3DH_REG_WHO_AM_I, &chip_id, 1) != 1)) {
        return;
    }
    
    if (!lis3dh_is_present()) {
        return;
    }

    /* CTRL_REG1: ODR[7:4] | LPen=0 | Zen=1 | Yen=1 | Xen=1 */
    uint8_t ctrl1 = ((odr & 0x0F) << 4) | 0x07;
    lis3dh_write_reg(LIS3DH_REG_CTRL1, ctrl1);
}

bool lis3dh_is_present()
{
    return chip_id == LIS3DH_WHO_AM_I_ID;
}

static struct {
    int16_t x;
    int16_t y;
    int16_t z;
    uint16_t angle;
} cache;

uint16_t lis3dh_read()
{
    return cache.angle;
}

int16_t lis3dh_read_x()
{
    return cache.x;
}

int16_t lis3dh_read_y()
{
    return cache.y;
}

int16_t lis3dh_read_z()
{
    return cache.z;
}

void lis3dh_update()
{
    if (!lis3dh_is_present()) {
        return;
    }

    /* Read X_L..Z_H in one burst (auto-increment via bit7) */
    uint8_t buf[6];
    if (lis3dh_read_bytes(LIS3DH_MULTI_READ(LIS3DH_REG_OUT_X_L), buf, 6) != 6) {
        return;
    }

    cache.x = (buf[1] << 8) | buf[0];
    cache.y = (buf[3] << 8) | buf[2];
    cache.z = (buf[5] << 8) | buf[4];

    /* atan2 -> [-π, π], normalize to [0, 2π), map to [0..4095] */
    float angle_rad = atan2f((float)cache.y, (float)cache.x);
    if (angle_rad < 0.0f) {
        angle_rad += 2.0f * (float)M_PI;
    }

    cache.angle = (uint16_t)(angle_rad * 4096.0f / 2.0f / (float)M_PI) & 0xFFF;
}
