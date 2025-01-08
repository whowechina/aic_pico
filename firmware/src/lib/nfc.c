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

#define DEBUG(...) { if (nfc_runtime.debug) printf(__VA_ARGS__); }

nfc_runtime_t nfc_runtime;

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

static const char *card_type_str[] = {
    "None",
    "MIFARE",
    "FeliCa",
    "15693"
};

const char *nfc_card_type_str(nfc_card_type card_type)
{
    if (card_type >= sizeof(card_type_str) / sizeof(card_type_str[0])) {
        return "Unknown";
    }
    return card_type_str[card_type];
}

static const char *card_name_str[] = {
    "None",
    "Amusement IC Generic",
    "Amusement IC SEGA",
    "Amusement IC KONAMI",
    "Amusement IC Bandai Namco",
    "Amusement IC NESiCA",
    "Amusement IC Virtual",
    "MIFARE Generic",
    "AIME",
    "Bandai Namco Passport",
    "NESiCA",
    "Vicinity Generic",
    "E-Amusement Pass",
};

const char *nfc_card_name_str(nfc_card_name card_name)
{
    if (card_name >= sizeof(card_name_str) / sizeof(card_name_str[0])) {
        return "None";
    }
    return card_name_str[card_name];
}

#define CARD_INFO_TIMEOUT_US (1000 * 1000)


static nfc_card_name last_card_name = CARD_NONE;
static bool last_name_final;
static uint64_t last_card_name_time = 0;
static card_name_listener_func card_name_listener;

static inline bool last_name_expired()
{
    return time_us_64() - last_card_name_time > CARD_INFO_TIMEOUT_US;
}

static void update_card_name(nfc_card_name card_name, bool final)
{
    bool accept = last_name_expired();

    if (!accept && !last_name_final) {
        accept = true;
    }

    if (!accept && (card_name == last_card_name)) {
        accept = true;
    }

    if (accept) {
        last_card_name = card_name;
        last_name_final = final;

        last_card_name_time = time_us_64();
        if (card_name_listener) {
            card_name_listener(card_name);
        }
    }
}

void nfc_set_card_name_listener(card_name_listener_func listener)
{
    card_name_listener = listener;
}

nfc_card_name nfc_last_card_name()
{
    if (last_name_expired()) {
        return CARD_NONE;
    }

    return last_card_name;
}

#define func_null NULL
struct {
    const char *(*firmware_ver)();
    bool (*poll_mifare)(uint8_t uid[7], int *len);
    bool (*poll_felica)(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache);
    bool (*poll_vicinity)(uint8_t uid[8]);
    void (*rf_field)(bool on);
    bool (*mifare_auth)(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t key[6]);
    bool (*mifare_read)(uint8_t block_id, uint8_t block_data[16]);
    bool (*felica_read)(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16]);
    void (*set_wait_loop)(nfc_wait_loop_t loop);
    void (*select)(int phase);
    void (*deselect)();
    bool (*iso15693_read)(const uint8_t uid[8], uint8_t block_id, uint8_t block_data[4]);
} api[3] = {
    {
        pn532_firmware_ver,
        pn532_poll_mifare, pn532_poll_felica, func_null,
        pn532_rf_field,
        pn532_mifare_auth, pn532_mifare_read,
        pn532_felica_read,
        pn532_set_wait_loop,
        pn532_select,
        pn532_deselect,
        func_null,
    },
    {
        pn5180_firmware_ver,
        pn5180_poll_mifare, pn5180_poll_felica, pn5180_poll_vicinity,
        pn5180_rf_field,
        pn5180_mifare_auth, pn5180_mifare_read,
        pn5180_felica_read,
        pn5180_set_wait_loop,
        pn5180_select,
        pn5180_deselect,
        pn5180_15693_read,
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
    spi_init(port, 8 * 1000 * 1000);
    spi_set_format(port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);

    nfc_attach_spi(port, rst, nss, busy);
}

void nfc_init()
{
    for (int retry = 0; retry < 3; retry++) {
        if (i2c.port && pn532_init(i2c.port)) {
            nfc_module = NFC_MODULE_PN532;
        } else if (spi.port && pn5180_init(spi.port, spi.rst, spi.nss, spi.busy)) {
            nfc_module = NFC_MODULE_PN5180;
        }
        if (nfc_module != NFC_MODULE_UNKNOWN) {
            break;
        }
        sleep_ms(200);
    }
}

void nfc_set_wait_loop(nfc_wait_loop_t loop)
{
    if (!api[nfc_module].set_wait_loop) {
        return;
    }
    api[nfc_module].set_wait_loop(loop);
}

const char *nfc_module_version()
{
    if (!api[nfc_module].firmware_ver) {
        return 0;
    }
    return api[nfc_module].firmware_ver();
}

void nfc_pn5180_tx_tweak(bool enable)
{
    nfc_runtime.pn5180_tx_tweak = enable;
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

static nfc_card_t last_card;
static uint64_t last_card_time = 0;

static void update_last_card(const nfc_card_t *card)
{
    last_card = *card;
    last_card_time = time_us_64();
}

nfc_card_t nfc_detect_card()
{
    nfc_card_t card = { 0 };

    if (nfc_detect_mifare(&card) ||
        nfc_detect_felica(&card) ||
        nfc_detect_vicinity(&card)) {

        update_last_card(&card);
        if (card.card_type == NFC_CARD_FELICA) {
            update_card_name(CARD_AIC, false);
        } else if (card.card_type == NFC_CARD_MIFARE) {
            update_card_name(CARD_MIFARE, false);
        } else if (card.card_type == NFC_CARD_VICINITY) {
            update_card_name(CARD_VICINITY, false);
        }

        return card;
    }

    card.card_type = NFC_CARD_NONE;
    return card;
}

nfc_card_t nfc_detect_card_ex(bool mifare, bool felica, bool vicinity)
{
    nfc_card_t card = { 0 };

    if ((mifare && nfc_detect_mifare(&card))
        || (felica && nfc_detect_felica(&card))
        || (vicinity && nfc_detect_vicinity(&card))) {
        update_last_card(&card);
        return card;
    }

    card.card_type = NFC_CARD_NONE;
    return card;
}

static void identify_felica()
{
    nfc_felica_read(0x000b, 0x8082, last_card.uid);
}

static void identify_mifare()
{
    uint8_t buf_ignored[16];
    if (nfc_mifare_read(0x00, buf_ignored)) {
        // could be a NESiCA
    } else if (nfc_mifare_auth(last_card.uid, 0x03, 0, 
                        (const uint8_t *)"\x60\x90\xD0\x06\x32\xF5")) {
        nfc_mifare_read(0x01, buf_ignored);
    } else {
        nfc_detect_card_ex(true, false, false);
        nfc_mifare_auth(last_card.uid, 0x03, 1,
                        (const uint8_t *)"WCCFv2");
        nfc_mifare_read(0x01, buf_ignored);
    }
}

static void identify_15693()
{
    uint8_t buf_ignored[16];
    nfc_15693_read(last_card.uid, 0x1b, buf_ignored);
}

void nfc_identify_last_card()
{
    if (time_us_64() - last_card_time > CARD_INFO_TIMEOUT_US) {
        DEBUG("\nLast card expired, no identification");
        return;
    }

    if (last_card.card_type == NFC_CARD_FELICA) {
        identify_felica();
    } else if (last_card.card_type == NFC_CARD_MIFARE) {
        identify_mifare();
    } else if (last_card.card_type == NFC_CARD_VICINITY) {
        identify_15693();
    }
}

void display_card(const nfc_card_t *card)
{
    if (card->card_type != NFC_CARD_NONE) {
        printf("\n%s:", nfc_card_type_str(card->card_type));
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
    DEBUG("\nAuth block %d key %d %.6s [", block_id, key_id, key);
    for (int i = 0; i < 6; i++) {
        DEBUG(" %02X", key[i]);
    }
    DEBUG(" ]");
    return api[nfc_module].mifare_auth(uid, block_id, key_id, key);
}

static void mifare_report_name(uint8_t block_id, const uint8_t block_data[16])
{
    if (block_id == 0) {
        if (memcmp(block_data + 10, "\xf8\x01", 2) == 0) {
            update_card_name(CARD_NESICA, true);
        }
    } else if (block_id == 1) {
        if (memcmp(block_data + 2, "NBGIC", 5) == 0) {
            update_card_name(CARD_BANA, true);
        } else if (memcmp(block_data, "SBSD", 4) == 0) {
            update_card_name(CARD_AIME, true);
        }
    }
}

bool nfc_mifare_read(uint8_t block_id, uint8_t block_data[16])
{
    if (!api[nfc_module].mifare_read) {
        return false;
    }
    
    bool read_ok = api[nfc_module].mifare_read(block_id, block_data);

    if (!read_ok) {
        return false;
    }

    mifare_report_name(block_id, block_data);

    return true;
}

static void felica_report_name(const uint8_t dfc[2])
{
    update_card_name(CARD_AIC, false);

    if (dfc[1] == 0x78) {
        update_card_name(CARD_AIC_SEGA, true);
    } else if (dfc[1] == 0x68) {
        update_card_name(CARD_AIC_KONAMI, true);
    } else if ((dfc[1] == 0x2a) || (dfc[1] == 0x3a)) {
        update_card_name(CARD_AIC_BANA, true);
    } else if (dfc[1] == 0x79) {
        update_card_name(CARD_AIC_NESICA, true);
    }
}

bool nfc_felica_read(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16])
{
    if (!api[nfc_module].felica_read) {
        return false;
    }
    
    bool read_ok = api[nfc_module].felica_read(svc_code, block_id, block_data);

    if (read_ok && (svc_code == 0x000b) && (block_id == 0x8082)) {
        felica_report_name(block_data + 8); // DFC
    }

    return read_ok;
}

void nfc_select(int phase)
{
    if (api[nfc_module].select) {
        api[nfc_module].select(phase);
    }
}

void nfc_deselect()
{
    if (api[nfc_module].deselect) {
        api[nfc_module].deselect();
    }
}

static void vicinity_report_name(uint8_t block_id, const uint8_t block_data[4])
{
    if ((block_id == 0x1b) && (memcmp(block_data, "W_OK", 4) == 0)) {
        update_card_name(CARD_EAMUSE, true);
    }
}

bool nfc_15693_read(const uint8_t uid[8], uint8_t block_id, uint8_t block_data[4])
{
    if (!api[nfc_module].iso15693_read) {
        return false;
    }
    
    bool read_ok = api[nfc_module].iso15693_read(uid, block_id, block_data);
    if (read_ok) {
        vicinity_report_name(block_id, block_data);
    }

    return read_ok;
}
