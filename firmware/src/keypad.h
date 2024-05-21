/*
 * AIC Pico Keypad
 * WHowe <github.com/whowechina>
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>
#include <stdbool.h>

void keypad_init();
uint8_t keypad_key_num();
bool keypad_is_stuck();
void keypad_update();
uint16_t keypad_read();

#endif
