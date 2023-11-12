/*
 * AIC Pico Keypad
 * WHowe <github.com/whowechina>
 * 
 */

#include "keypad.h"

#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#include "board_defs.h"

static const uint8_t keypad_gpio[] = KEYPAD_DEF;

#define KEY_NUM (sizeof(keypad_gpio))

static bool sw_val[KEY_NUM]; /* true if pressed */
static uint64_t sw_freeze_time[KEY_NUM];

void keypad_init()
{
    for (int i = 0; i < KEY_NUM; i++)
    {
        sw_val[i] = false;
        sw_freeze_time[i] = 0;
        int8_t gpio = keypad_gpio[i];
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
}

uint8_t keypad_key_num()
{
    return KEY_NUM;
}

static uint16_t keypad_reading;

/* If a switch flips, it freezes for a while */
#define DEBOUNCE_FREEZE_TIME_US 20000
void keypad_update()
{
    uint64_t now = time_us_64();
    uint16_t keys = 0;

    for (int i = KEY_NUM - 1; i >= 0; i--) {
        bool sw_pressed = !gpio_get(keypad_gpio[i]);
        
        if (now >= sw_freeze_time[i]) {
            if (sw_pressed != sw_val[i]) {
                sw_val[i] = sw_pressed;
                sw_freeze_time[i] = now + DEBOUNCE_FREEZE_TIME_US;
            }
        }

        keys <<= 1;
        if (sw_val[i]) {
            keys |= 1;
        }
    }

    keypad_reading = keys;
}

uint16_t keypad_read()
{
    return keypad_reading;
}
