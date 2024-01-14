/*
 * NFC Reader Interface
 * WHowe <github.com/whowechina>
 * 
 */

#ifndef NFC_H
#define NFC_H

#include "hardware/i2c.h"

typedef enum {
    NFC_CARD_NULL = 0,
    NFC_CARD_MIFARE,
    NFC_CARD_FELICA,
    NFC_CARD_VICINITY,
} nfc_card_type;

typedef void (*nfc_wait_loop_t)();
typedef struct {
    nfc_card_type card_type;
    uint16_t len;
    uint8_t uid[8];
    uint8_t pmm[8];
    uint8_t syscode[2];
} nfc_card_t;

void nfc_init(nfc_wait_loop_t loop);
nfc_card_t nfc_detect_card();

#endif
