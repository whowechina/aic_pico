/*
 * AIME Reader
 * WHowe <github.com/whowechina>
 */

#ifndef AIME_H
#define AIME_H

/* return true if accepts a byte, false if rejects */
typedef void (*aime_putc_func)(uint8_t byte);

void aime_init(aime_putc_func putc_func);
bool aime_feed(int c);

uint32_t aime_led_color();

// mode 0 or 1
void aime_set_baudrate(int mode);
void aime_set_virtual_aic(bool enable);

#endif
