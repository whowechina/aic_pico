/*
 * AIC Reader Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_AIC_PICO

#define I2C_PORT_LIST { { i2c0, 21, 20 }, { i2c1, 19, 18 }, { i2c0, 17, 16 }, { i2c1, 27, 26 } }
#define I2C_FREQ 433*1000

#define SPI_PORT spi0
#define SPI_MISO 16
#define SPI_SCK 18
#define SPI_MOSI 19
#define SPI_RST 27
#define SPI_NSS 17
#define SPI_BUSY 26

#define RGB_PIN 12
#define DEFAULT_RGB_ORDER GRB
#define LED_DEF { 25, 22, 13, 15 }

#define KEYPAD_DEF { 6, 7, 8, 3, 4, 5, 0, 1, 2, 9, 10, 11 }
#define AIC_TOUCH_EN 14

/* HID Keycode: https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h */
// Numpad: 1234567890-.
#define KEYPAD_NKRO_MAP "\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x56\x63"

#else

#endif
