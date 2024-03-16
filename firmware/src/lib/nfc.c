/*
 * NFC Reader Interface to real modules
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/i2c.h"
#include "hardware/gpio.h"

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

#define func_null NULL
struct {
    bool (*poll_mifare)(uint8_t uid[7], int *len);
    bool (*poll_felica)(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache);
    bool (*poll_vicinity)(uint8_t uid[8]);
    void (*rf_field)(bool on);
    bool (*mifare_auth)(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t *key);
    bool (*mifare_read)(uint8_t block_id, uint8_t block_data[16]);
    void (*set_wait_loop)(nfc_wait_loop_t loop);
} api[3] = {
    {
        pn532_poll_mifare, pn532_poll_felica, func_null,
        pn532_rf_field,
        pn532_mifare_auth, pn532_mifare_read,
        pn532_set_wait_loop
    },
    {
        pn5180_poll_mifare, pn5180_poll_felica, pn5180_poll_vicinity,
        func_null,
        func_null, func_null,
        pn5180_set_wait_loop
    },
    { 0 },
};

static struct {
    i2c_inst_t *port;
} i2c = {0};

void nfc_attach_i2c(i2c_inst_t *port)
{
    i2c.port = port;
}

void nfc_init_i2c(i2c_inst_t *port, uint8_t scl, uint8_t sda, uint32_t freq)
{
    i2c_init(port, freq);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_pull_up(scl);
    gpio_pull_up(sda);

    nfc_attach_i2c(port);
}

static struct {
    spi_inst_t *port;
    uint8_t rst;
    uint8_t nss;
    uint8_t busy;
} spi = {0};

void nfc_attach_spi(spi_inst_t *port, uint8_t rst, uint8_t nss, uint8_t busy)
{
    spi.port = port;
    spi.rst = rst;
    spi.nss = nss;
    spi.busy = busy;
}

void nfc_init_spi(spi_inst_t *port, uint8_t miso, uint8_t sck, uint8_t mosi,
                 uint8_t rst, uint8_t nss, uint8_t busy)
{
    spi_init(port, 2000 * 1000);
    spi_set_format(port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);

    nfc_attach_spi(port, rst, nss, busy);
}

void nfc_init()
{
    if (i2c.port && pn532_init(i2c.port)) {
        nfc_module = NFC_MODULE_PN532;
    } else if (spi.port && pn5180_init(spi.port, spi.rst, spi.nss, spi.busy)) {
        nfc_module = NFC_MODULE_PN5180;
    }
}

void nfc_set_wait_loop(nfc_wait_loop_t loop)
{
    if (!api[nfc_module].set_wait_loop) {
        return;
    }
    api[nfc_module].set_wait_loop(loop);
}

static bool nfc_detect_mifare(nfc_card_t *card)
{
    uint8_t id[20] = { 0 };
    int len = sizeof(id);

    if (!api[nfc_module].poll_mifare ||
        !api[nfc_module].poll_mifare(id, &len)) {
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

    if (!api[nfc_module].poll_felica ||
        !api[nfc_module].poll_felica(id, id + 8, id + 16, false)) {
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

    if (!api[nfc_module].poll_vicinity ||
        !api[nfc_module].poll_vicinity(id)) {
        return false;
    }

    card->card_type = NFC_CARD_VICINITY;
    card->len = 8;
    memcpy(card->uid, id, 8);

    return true;
}

void nfc_rf_field(bool on)
{
    if (api[nfc_module].rf_field) {
        api[nfc_module].rf_field(on);
    }
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
    if (!api[nfc_module].mifare_auth) {
        return false;
    }
    return api[nfc_module].mifare_auth(uid, block_id, key_id, key);
}

bool nfc_mifare_read(uint8_t block_id, uint8_t block_data[16])
{
    if (!api[nfc_module].mifare_read) {
        return false;
    }
    return api[nfc_module].mifare_read(block_id, block_data);
}
