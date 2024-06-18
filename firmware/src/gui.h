
/*
 * GUI on a Touch Display (ST7789 + CST816)
 * WHowe <github.com/whowechina>
 * 
 */

#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stdbool.h>

#include "nfc.h"

void gui_init();
void gui_level(uint8_t level);
void gui_loop();
uint16_t gui_keypad_read();
void gui_report_card(nfc_card_name card);

#endif