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
static bool keypad_en = true;
static bool keystuck = false;

void keypad_init()
{
    gpio_init(KEYPAD_EN);
    gpio_set_function(KEYPAD_EN, GPIO_FUNC_SIO);
    gpio_set_dir(KEYPAD_EN, GPIO_IN);
    gpio_pull_up(KEYPAD_EN);
    keypad_en = gpio_get(KEYPAD_EN);
    if (!keypad_en) {
        return;
    }

    for (int i = 0; i < KEY_NUM; i++)
    {
        sw_val[i] = false;
        sw_freeze_time[i] = 0;
        int8_t gpio = keypad_gpio[i];
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
        if (gpio_get(gpio) == 0) {
            keystuck = true;
            break;
        }
    }
}

uint8_t keypad_key_num()
{
    return keypad_en ? KEY_NUM : 0;
}

static uint16_t keypad_reading;

/* If a switch flips, it freezes for a while */
#define DEBOUNCE_FREEZE_TIME_US 20000
void keypad_update()
{
    if (!keypad_en || keystuck) {
        return;
    }

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

bool keypad_is_stuck()
{
    return keystuck;
}
