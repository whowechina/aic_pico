/*
 * NFC Reader Interface
 * WHowe <github.com/whowechina>
 * 
 */

#ifndef NFC_H
#define NFC_H

#include "hardware/i2c.h"

typedef enum {
    NFC_CARD_NONE = 0,
    NFC_CARD_MIFARE,
    NFC_CARD_FELICA,
    NFC_CARD_VICINITY,
} nfc_card_type;

const char *nfc_card_name(nfc_card_type card_type);

typedef void (*nfc_wait_loop_t)();
typedef struct {
    nfc_card_type card_type;
    uint16_t len;
    union {
        uint8_t uid[8];
        uint8_t idm[8];
    };
    uint8_t pmm[8];
    uint8_t syscode[2];
} nfc_card_t;

void nfc_init(i2c_inst_t *i2c, uint8_t scl, uint8_t sda, uint32_t freq);
void nfc_set_wait_loop(nfc_wait_loop_t loop);

void nfc_rf_field(bool on);

nfc_card_t nfc_detect_card();
void display_card(const nfc_card_t *card);

nfc_card_t nfc_poll_felica();

const char *nfc_module_name();

bool nfc_mifare_auth(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t *key);
bool nfc_mifare_read(uint8_t block_id, uint8_t block_data[16]);

bool nfc_felica_read_wo_encrypt(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16]);
bool nfc_felica_write_wo_encrypt(uint16_t svc_code, uint16_t block_id, const uint8_t block_data[16]);

#endif
