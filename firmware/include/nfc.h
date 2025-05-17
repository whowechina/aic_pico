/*
 * NFC Operations
 * WHowe <github.com/whowechina>
 * 
 */

#ifndef NFC_H
#define NFC_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "hardware/spi.h"

typedef enum {
    NFC_CARD_NONE = 0,
    NFC_CARD_MIFARE,
    NFC_CARD_FELICA,
    NFC_CARD_VICINITY,
} nfc_card_type;

const char *nfc_card_type_str(nfc_card_type card_type);

typedef enum {
    CARD_NONE,
    CARD_AIC,
    CARD_AIC_SEGA,
    CARD_AIC_KONAMI,
    CARD_AIC_BANA,
    CARD_AIC_NESICA,
    CARD_AIC_VIRTUAL,
    CARD_SUICA,
    CARD_MIFARE,
    CARD_AIME,
    CARD_BANA,
    CARD_NESICA,
    CARD_VICINITY,
    CARD_EAMUSE,
} nfc_card_name;

const char *nfc_card_name_str(nfc_card_name card_name);
nfc_card_name nfc_last_card_name();
void nfc_identify_last_card();

typedef void (*card_name_listener_func)(nfc_card_name card_name);
void nfc_set_card_name_listener(card_name_listener_func listener);

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

typedef struct {
    bool debug;
    bool pn5180_tx_tweak;
} nfc_runtime_t;

extern nfc_runtime_t nfc_runtime;

bool nfc_init_i2c(i2c_inst_t *port, uint8_t scl, uint8_t sda, uint32_t freq);
bool nfc_init_spi(spi_inst_t *port, uint8_t miso, uint8_t sck, uint8_t mosi,
                  uint8_t rst, uint8_t nss, uint8_t busy);

/* port and gpio should be initialized before attach */
void nfc_attach_i2c(i2c_inst_t *port);
void nfc_attach_spi(spi_inst_t *port, uint8_t rst, uint8_t nss, uint8_t busy);
/* should attach i2c or spi port before nfc_init() */
bool nfc_init();


/* should be called only after init */
void nfc_set_wait_loop(nfc_wait_loop_t loop);

void nfc_pn5180_tx_tweak(bool enable);

void nfc_rf_field(bool on);

nfc_card_t nfc_detect_card();
nfc_card_t nfc_detect_card_ex(bool mifare, bool felica, bool vicinity);

void display_card(const nfc_card_t *card);

const char *nfc_module_name();
const char *nfc_module_version();

bool nfc_mifare_auth(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t *key);
bool nfc_mifare_read(uint8_t block_id, uint8_t block_data[16]);

bool nfc_felica_read(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16]);

bool nfc_15693_read(const uint8_t uid[8], uint8_t block_id, uint8_t block_data[4]);

void nfc_select(int phase);
void nfc_deselect();

#endif
