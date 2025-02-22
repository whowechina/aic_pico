/*
 * Cardio processing and auto PIN-entry
 * WHowe <github.com/whowechina>
 */

#ifndef CARDIO_H
#define CARDIO_H

void cardio_run(bool hid_light);
void cardio_clear();
void cardio_report_cardio();
bool cardio_get_last(uint8_t uid[8], uint8_t *len);
int cardio_get_pin_key();

#endif