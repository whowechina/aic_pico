/*
 * NFC Reader Interface to real modules
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_defs.h"

#include "nfc.h"
#include "pn532.h"
#include "pn5180.h"


static enum {
    NFC_MODULE_PN532 = 0,
    NFC_MODULE_PN5180,
    NFC_MODULE_UNKNOWN,
} nfc_module = NFC_MODULE_UNKNOWN;

static const char *nfc_module_names[] = {
    "PN532",
    "PN5180",
    "Unknown",
};

const char *nfc_module_name()
{
    return nfc_module_names[nfc_module];
}

static const char *nfc_card_names[] = {
    "None",
    "MIFARE",
    "FeliCa",
    "15693"
};

const char *nfc_card_name(nfc_card_type card_type)
{
    if (card_type >= sizeof(nfc_card_names) / sizeof(nfc_card_names[0])) {
        return "Unknown";
    }
    return nfc_card_names[card_type];
}

static bool null_poll_mifare(uint8_t uid[7], int *len)
{
    return false;
}

static bool null_poll_felica(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache)
{
    return false;
}

static bool null_poll_vicinity(uint8_t uid[8])
{
    return false;
}

static void null_rf_field(bool on)
{
}

struct {
    bool (*poll_mifare)(uint8_t uid[7], int *len);
    bool (*poll_felica)(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache);
    bool (*poll_vicinity)(uint8_t uid[8]);
    void (*rf_field)(bool on);
} api[3] = {
    { pn532_poll_mifare, pn532_poll_felica, null_poll_vicinity, pn532_rf_field},
    { pn5180_poll_mifare, pn5180_poll_felica, pn5180_poll_vicinity, null_rf_field},
    { null_poll_mifare, null_poll_felica, null_poll_vicinity, null_rf_field},
};

void nfc_init(nfc_wait_loop_t loop)
{
    if (pn532_init(I2C_PORT, I2C_SCL, I2C_SDA, I2C_FREQ)) {
        nfc_module = NFC_MODULE_PN532;
        pn532_set_wait_loop(loop);
    } else if (pn5180_init(spi0, 16, 18, 19, 27, 17, 26)) {
        nfc_module = NFC_MODULE_PN5180;
        pn5180_set_wait_loop(loop);
    }
}

static bool nfc_detect_mifare(nfc_card_t *card)
{
    uint8_t id[20] = { 0 };
    int len = sizeof(id);

    if (!api[nfc_module].poll_mifare(id, &len)) {
        return false;
    }

    card->card_type = NFC_CARD_MIFARE;
    card->len = len;
    memcpy(card->uid, id, len);

    return true;
}

static bool nfc_detect_felica(nfc_card_t *card)
{
    uint8_t id[20] = { 0 };

    if (!api[nfc_module].poll_felica(id, id + 8, id + 16, false)) {
        return false;
    }

    card->card_type = NFC_CARD_FELICA;
    card->len = 8;
    memcpy(card->uid, id, 8);
    memcpy(card->pmm, id + 8, 8);
    memcpy(card->syscode, id + 16, 2);

    return true;
}

static bool nfc_detect_vicinity(nfc_card_t *card)
{
    uint8_t id[8] = { 0 };

    if (!api[nfc_module].poll_vicinity(id)) {
        return false;
    }

    card->card_type = NFC_CARD_VICINITY;
    card->len = 8;
    memcpy(card->uid, id, 8);

    return true;
}

void nfc_rf_field(bool on)
{
    api[nfc_module].rf_field(on);
}

nfc_card_t nfc_detect_card()
{
    nfc_card_t card = { 0 };

    if (!nfc_detect_mifare(&card) &&
        !nfc_detect_felica(&card) &&
        !nfc_detect_vicinity(&card)) {
        card.card_type = NFC_CARD_NONE;
    }

    return card;
}

nfc_card_t nfc_poll_felica()
{
    nfc_card_t card = { 0 };
    if (!nfc_detect_felica(&card)) {
        card.card_type = NFC_CARD_NONE;
    }
    return card;
}

void display_card(const nfc_card_t *card)
{
    if (card->card_type != NFC_CARD_NONE) {
        printf("\n%s:", nfc_card_name(card->card_type));
        for (int i = 0; i < card->len; i++) {
            printf(" %02X", card->uid[i]);
        }
    }
}

bool nfc_mifare_auth(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t *key)
{
    if (nfc_module == NFC_MODULE_PN532) {
        return pn532_mifare_auth(uid, block_id, key_id, key);
    } else if (nfc_module == NFC_MODULE_PN5180) {
        return false;//pn5180_mifare_auth(uid, block_id, key_id, key);
    }
    return false;
}

bool nfc_mifare_read(uint8_t block_id, uint8_t block_data[16])
{
    if (nfc_module == NFC_MODULE_PN532) {
        return pn532_mifare_read(block_id, block_data);
    } else if (nfc_module == NFC_MODULE_PN5180) {
        return false; // pn5180_mifare_read(block_id, block_data);
    }
    return false;
}

bool nfc_felica_read_wo_encrypt(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16])
{
    if (nfc_module == NFC_MODULE_PN532) {
        return pn532_felica_read_wo_encrypt(svc_code, block_id, block_data);
    } else if (nfc_module == NFC_MODULE_PN5180) {
        return false; //pn5180_felica_read_wo_encrypt(svc_code, block_id, block_data);
    }
    return false;
}

bool nfc_felica_write_wo_encrypt(uint16_t svc_code, uint16_t block_id, const uint8_t block_data[16])
{
    if (nfc_module == NFC_MODULE_PN532) {
        return pn532_felica_write_wo_encrypt(svc_code, block_id, block_data);
    } else if (nfc_module == NFC_MODULE_PN5180) {
        return false; //pn5180_felica_write_wo_encrypt(svc_code, block_id, block_data);
    }
    return false;
}
