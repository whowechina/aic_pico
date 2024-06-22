/*
 * Bandai Namco Protocol
 * WHowe <github.com/whowechina>
 */

#ifndef BANA_H
#define BANA_H

#include <stdint.h>
#include <stdbool.h>

/* return true if accepts a byte, false if rejects */
typedef void (*bana_putc_func)(uint8_t byte);

void bana_init(bana_putc_func putc_func);

bool bana_feed(int c);

/* if bana is currently active */
bool bana_is_active();

void bana_dtr_off();

const char *bana_get_led_pattern();

#endif
