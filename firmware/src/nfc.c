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

struct {
    bool (*poll_mifare)(uint8_t uid[7], int *len);
    bool (*poll_felica)(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache);
    bool (*poll_vicinity)(uint8_t uid[8]);
} api[3] = {
    { pn532_poll_mifare, pn532_poll_felica, null_poll_vicinity,},
    { pn5180_poll_mifare, pn5180_poll_felica, pn5180_poll_vicinity,},
    { null_poll_mifare, null_poll_felica, null_poll_vicinity,},
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

static void nfc_config_sam()
{
    if (nfc_module == NFC_MODULE_PN532) {
        pn532_config_sam();
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

nfc_card_t nfc_detect_card()
{
    nfc_card_t card = { 0 };

    nfc_config_sam();

    if (!nfc_detect_mifare(&card) &&
        !nfc_detect_felica(&card) &&
        !nfc_detect_vicinity(&card)) {
        card.card_type = NFC_CARD_NULL;
    }

    return card;
}