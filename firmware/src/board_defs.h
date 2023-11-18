/*
 * AIC Reader Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_AIC_PICO

#define I2C_PORT i2c0
#define I2C_SCL 21
#define I2C_SDA 20
#define I2C_FREQ 733*1000

#define RGB_PIN 12
#define RGB_ORDER GRB // or RGB
#define LED_DEF { 25, 22, 13, 15 }

#define KEYPAD_DEF { 6, 7, 8, 3, 4, 5, 0, 1, 2, 9, 10, 11 }
/* HID Keycode: https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h */
// Numpad: 1234567890-.
#define KEYPAD_NKRO_MAP "\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x56\x63"

#else

#endif
