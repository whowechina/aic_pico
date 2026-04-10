/*
 * LIS3DH 3-Axis Accelerometer
 * I2C Interface, AD0=High (addr 0x19)
 * WHowe <github.com/whowechina>
 */

#ifndef LIS3DH_H
#define LIS3DH_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

/* odr: ODR field value for CTRL_REG1[7:4]
 *   0x0 = power-down, 0x1 = 1Hz, 0x2 = 10Hz, 0x3 = 25Hz,
 *   0x4 = 50Hz,       0x5 = 100Hz, 0x6 = 200Hz, 0x7 = 400Hz */
void lis3dh_init(i2c_inst_t *i2c_port, uint8_t odr);
bool lis3dh_is_present();

uint16_t lis3dh_read(); // XY gravity angle, 12-bit [0..4095] = [0..360deg)

int16_t lis3dh_read_x();
int16_t lis3dh_read_y();
int16_t lis3dh_read_z();

void lis3dh_update();

#endif
