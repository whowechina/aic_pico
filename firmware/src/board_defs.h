/*
 * AIC Reader Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_AIC_PICO

#define I2C_PORT i2c0
#define I2C_SCL 21
#define I2C_SDA 20
#define I2C_FREQ 733*1000

#define RGB_PIN 22
#define RGB_ORDER GRB // or RGB

#else

#endif
