/*
 * AIME Reader
 * WHowe <github.com/whowechina>
 * 
 * Use NFC Module to read AIME
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "nfc.h"
#include "aime.h"

static bool debug = false;
#define DEBUG(...) if (nfc_runtime.debug) printf(__VA_ARGS__)

#define AIME_EXPIRE_US (1200 * 1000000ULL)
#define AIME_FAST_EXPIRE_US (3 * 1000000ULL)

enum {
    CMD_GET_FW_VERSION = 0x30,
    CMD_GET_HW_VERSION = 0x32,

    // Card read
    CMD_START_POLLING = 0x40,
    CMD_STOP_POLLING = 0x41,
    CMD_CARD_DETECT = 0x42,
    CMD_CARD_SELECT = 0x43,
    CMD_CARD_HALT = 0x44,

    // MIFARE
    CMD_MIFARE_KEY_SET_A = 0x50,
    CMD_MIFARE_AUTHORIZE_A = 0x51,
    CMD_MIFARE_READ = 0x52,
    CMD_MIFARE_WRITE = 0x53,
    CMD_MIFARE_KEY_SET_B = 0x54,
    CMD_MIFARE_AUTHORIZE_B = 0x55,

    // Boot,update
    CMD_TO_UPDATER_MODE = 0x60,
    CMD_SEND_HEX_DATA = 0x61,
    CMD_TO_NORMAL_MODE = 0x62,
    CMD_SEND_BINDATA_INIT = 0x63,
    CMD_SEND_BINDATA_EXEC = 0x64,

    // FeliCa
    CMD_FELICA_PUSH = 0x70,
    CMD_FELICA_OP = 0x71,

    // LED board
    CMD_EXT_BOARD_LED_RGB = 0x81,
    CMD_EXT_BOARD_INFO = 0xf0,
    CMD_EXT_TO_NORMAL_MODE = 0xf5,
};

enum {
    STATUS_OK = 0,
    STATUS_INVALID_COMMAND = 3,
};

const char *fw_version[] = { "TN32MSEC003S F/W Ver1.2", "\x94" };
const char *hw_version[] = { "TN32MSEC003S H/W Ver3.0", "837-15396" };
const char *led_info[] = { "15084\xFF\x10\x00\x12", "000-00000\xFF\x11\x40" };
static int ver_mode = 1;
static bool expecting_dtr_off = false;
static uint64_t expected_dtr_off_time = 0;

static struct {
    bool enabled;
    bool active; // currently active
    uint8_t idm[8];
    const uint8_t pmm[8];
    const uint8_t syscode[2];
} virtual_aic = { false, false, 
                  "", "\x00\xf1\x00\x00\x00\x01\x43\x00", "\x88\xb4" };

static void putc_trap(uint8_t byte)
{
}

static aime_putc_func aime_putc = putc_trap;

void aime_sub_mode(int sub_mode)
{
    ver_mode = (sub_mode == 0) ? 0 : 1;
}

const char *aime_get_mode_string()
{
    return hw_version[ver_mode];
}

void aime_init(aime_putc_func putc_func)
{
    aime_putc = putc_func;
}

void aime_virtual_aic(bool enable)
{
    virtual_aic.enabled = enable;
}

static uint8_t mifare_keys[2][6]; // 'KeyA' and 'KeyB'

static union __attribute__((packed)) {
    struct {
        uint8_t len;
        uint8_t addr;
        uint8_t seq;
        uint8_t cmd;
        uint8_t status;
        uint8_t payload_len;
        uint8_t payload[];
    };
    uint8_t raw[256];
} response;

static union __attribute__((packed)) {
    struct {
        uint8_t len;
        uint8_t addr;
        uint8_t seq;
        uint8_t cmd;
        uint8_t payload_len;
        union {
            struct {
                uint8_t uid[4];
                uint8_t block_id;
            } mifare;
            struct {
                uint8_t idm[8];
                uint8_t len;
                uint8_t code;
                uint8_t data[0];
            } felica;
            uint8_t payload[250];
        };
    };
    uint8_t raw[256];
} request;

static struct {
    bool active;
    uint8_t len;
    uint8_t check_sum;
    bool escaping;
    uint64_t time;
} req_ctx;

static void build_response(int payload_len)
{
    response.len = payload_len + 6;
    response.addr = request.addr;
    response.seq = request.seq;
    response.cmd = request.cmd;
    response.status = STATUS_OK;
    response.payload_len = payload_len;
}

static void send_response()
{
    uint8_t checksum = 0;
    for (int i = 0; i < response.len; i++) {
        checksum += response.raw[i];
    }
    response.raw[response.len] = checksum;

    aime_putc(0xe0); // sync

    for (int i = 0; i < response.len + 1; i++) {
        uint8_t c = response.raw[i];
        if (c == 0xe0 || c == 0xd0) {
            aime_putc(0xd0); // escape
            c--;
        }
        aime_putc(c);
    }

    DEBUG("\n\033[33m%6ld<< %02x:", time_us_32() / 1000, response.cmd);
    for (int i = 0; i < response.payload_len; i++) {
        DEBUG(" %02x", response.payload[i]);
    }
    DEBUG("\033[0m");
}

static void send_simple_response(uint8_t status)
{
    build_response(0);
    response.status = status;
    send_response();
}

static void cmd_to_normal_mode()
{
    send_simple_response(STATUS_INVALID_COMMAND);
}

static void cmd_fake_version(const char *version[])
{
    int len = strlen(version[ver_mode]);
    build_response(len);
    memcpy(response.payload, version[ver_mode], len);
    send_response();
}

static void cmd_key_set(uint8_t key[6])
{
    memcpy(key, request.payload, 6);
    send_simple_response(STATUS_OK);
}

static void cmd_set_polling(bool enabled)
{
    nfc_rf_field(enabled);
    send_simple_response(STATUS_OK);
}

typedef struct __attribute__((packed)) {
    uint8_t count;
    uint8_t type;
    uint8_t id_len;
    union {
        struct {
            uint8_t idm[8];
            uint8_t pmm[8];
        };
        uint8_t uid[6];
    };
} card_info_t;

static void handle_mifare_card(const uint8_t *uid, int len)
{
    card_info_t *card = (card_info_t *) response.payload;

    build_response(len > 4 ? 10 : 7);

    card->count = 1;
    card->type = 0x10;
    card->id_len = len;
    memcpy(card->uid, uid, len);

    printf("\nMIFARE Card:");
    for (int i = 0; i < len; i++) {
        printf(" %02x", uid[i]);
    }
}

static void handle_felica_card(const uint8_t idm[8], const uint8_t pmm[8])
{
    build_response(19);
    card_info_t *card = (card_info_t *) response.payload;

    card->count = 1;
    card->type = 0x20;
    card->id_len = 16;
    memcpy(card->idm, idm, 8);
    memcpy(card->pmm, pmm, 8);
}

static void fake_felica_card()
{
    build_response(19);
    card_info_t *card = (card_info_t *) response.payload;

    card->count = 1;
    card->type = 0x20;
    card->id_len = 16;
    memcpy(card->idm, virtual_aic.idm, 8);
    memcpy(card->pmm, virtual_aic.pmm, 8);
}

static void handle_no_card()
{
    build_response(1);
    card_info_t *card = (card_info_t *) response.payload;

    card->count = 0;
    response.status = STATUS_OK;
}

static void cmd_detect_card()
{
    nfc_card_t card = nfc_detect_card();
    if (debug) {
        display_card(&card);
    }

    switch (card.card_type) {
        case NFC_CARD_MIFARE:
            if (virtual_aic.enabled) {
                printf("\nVirtual FeliCa from MIFARE.");
                virtual_aic.active = true;
                memcpy(virtual_aic.idm, "\x01\x01", 2);
                if (card.len == 4) {
                    memcpy(virtual_aic.idm + 2, card.uid, 4);
                    memcpy(virtual_aic.idm + 6, card.uid, 2);
                } else if (card.len == 7) {
                    memcpy(virtual_aic.idm + 2, card.uid, 6);
                }
                fake_felica_card();
            } else {
                handle_mifare_card(card.uid, card.len);
            }
            break;
        case NFC_CARD_FELICA:
            if (virtual_aic.enabled) {
                printf("\nVirtual FeliCa from FeliCa.");
                virtual_aic.active = true;
                memcpy(virtual_aic.idm, card.uid, 8);
                fake_felica_card();
            } else {
                handle_felica_card(card.uid, card.pmm);
            }
            break;
        case NFC_CARD_VICINITY:
            if (virtual_aic.enabled) {
                printf("\nVirtual FeliCa from 15693.");
                virtual_aic.active = true;
                memcpy(virtual_aic.idm, card.uid, 8);
                virtual_aic.idm[0] = 0x01;
                fake_felica_card();
            }
            break;
        default:
            handle_no_card();
            break;
    }

    send_response();
    if (card.card_type != NFC_CARD_NONE) {
        nfc_identify_last_card();
    }
}

static void cmd_card_select()
{
    send_simple_response(STATUS_OK);
}

static void cmd_mifare_auth(int type)
{
    const uint8_t *key = mifare_keys[type];
    nfc_mifare_auth(request.mifare.uid, request.mifare.block_id,
                    type, key);
    send_simple_response(STATUS_OK);
}

static void cmd_mifare_read()
{
    build_response(16);
    memset(response.payload, 0, 16);
    nfc_mifare_read(request.mifare.block_id, response.payload);
    send_response();
}

static void cmd_mifare_halt()
{
    send_simple_response(STATUS_OK);
}

static void cmd_felica()
{
    send_simple_response(STATUS_INVALID_COMMAND);
    expecting_dtr_off = true;
    expected_dtr_off_time = time_us_64() + 50000ULL;
}

static uint32_t led_color;

static void cmd_led_rgb()
{
    uint8_t r = request.payload[0];
    uint8_t g = request.payload[1];
    uint8_t b = request.payload[2];
    led_color = r << 16 | g << 8 | b;

    build_response(0);
    send_response();

    expecting_dtr_off = true;
    expected_dtr_off_time = time_us_64() + 400000ULL;
}

static void handle_frame()
{
    DEBUG("\n\033[32mAime %d:%02x >>", request.payload_len, request.cmd);
    for (int i = 0; i < request.payload_len; i++) {
        DEBUG(" %02x", request.payload[i]);
    }
    DEBUG("\033[0m");

    switch (request.cmd) {
        case CMD_TO_NORMAL_MODE:
            DEBUG("\nAIME: cmd_to_normal");
            cmd_to_normal_mode();
            break;
        case CMD_GET_FW_VERSION:
            DEBUG("\nAIME: fw_version");
            cmd_fake_version(fw_version);
            break;
        case CMD_GET_HW_VERSION:
            DEBUG("\nAIME: hw_version");
            cmd_fake_version(hw_version);
            break;
        case CMD_MIFARE_KEY_SET_A:
            DEBUG("\nAIME: key A");
            cmd_key_set(mifare_keys[0]);
            break;
        case CMD_MIFARE_KEY_SET_B:
            DEBUG("\nAIME: key B");
            cmd_key_set(mifare_keys[1]);
            break;

        case CMD_START_POLLING:
            cmd_set_polling(true);
            break;
        case CMD_STOP_POLLING:
            cmd_set_polling(false);
            break;
        case CMD_CARD_DETECT:
            cmd_detect_card();
            break;

        case CMD_FELICA_PUSH:
        case CMD_FELICA_OP:
            DEBUG("\nAIME: felica op");
            cmd_felica();
            break;

        case CMD_CARD_SELECT:
            DEBUG("\nAIME: card select");
            cmd_card_select();
            break;
        
        case CMD_MIFARE_AUTHORIZE_A:
            DEBUG("\nAIME: auth A");
            cmd_mifare_auth(0);
            break;

        case CMD_MIFARE_AUTHORIZE_B:
            DEBUG("\nAIME: auth B");
            cmd_mifare_auth(1);
            break;
        
        case CMD_MIFARE_READ:
            DEBUG("\nAIME: mifare read");
            cmd_mifare_read();
            break;

        case CMD_CARD_HALT:
            DEBUG("\nAIME: mifare halt");
            cmd_mifare_halt();
            break;

        case CMD_EXT_BOARD_INFO:
            DEBUG("\nAIME: led info");
            cmd_fake_version(led_info);
            break;
        case CMD_EXT_BOARD_LED_RGB:
            DEBUG("\nAIME: led rgb");
            cmd_led_rgb();
            break;

        case CMD_SEND_HEX_DATA:
        case CMD_EXT_TO_NORMAL_MODE:
            DEBUG("\nAIME: hex data or ex to normal: %d", request.cmd);
            send_simple_response(STATUS_OK);
            break;

        default:
            DEBUG("\nUnknown command: %02x [", request.cmd);
            for (int i = 0; i < request.len; i++) {
                DEBUG(" %02x", request.raw[i]);
            }
            DEBUG("]");
            send_simple_response(STATUS_OK);
            break;
    }
}

static uint64_t expire_time;

bool aime_feed(int c)
{
    if (c == 0xe0) {
        req_ctx.active = true;
        req_ctx.len = 0;
        req_ctx.check_sum = 0;
        req_ctx.escaping = false;
        req_ctx.time = time_us_64();
        return true;
    }

    if (!req_ctx.active) {
        return false;
    }

    if (c == 0xd0) {
        req_ctx.escaping = true;
        return true;
    }

    if (req_ctx.escaping) {
        c++;
        req_ctx.escaping = false;
    }

    if (req_ctx.len != 0 && req_ctx.len == request.len) {
        if (req_ctx.check_sum == c) {
            handle_frame();
            req_ctx.active = false;
            expire_time = time_us_64() + AIME_EXPIRE_US;
        }
        return true;
    }

    request.raw[req_ctx.len] = c;
    req_ctx.len++;
    req_ctx.check_sum += c;

    return true;
}

bool aime_is_active()
{
    return time_us_64() < expire_time;
}

void aime_dtr_off()
{
    if ((expecting_dtr_off) &&
        (abs(time_us_64() - expected_dtr_off_time) < 70000ULL)) {
        expecting_dtr_off = false;
        return;
    }

    if (!aime_is_active()) {
        return;
    }

    DEBUG("\nAIME: DTR_OFF delta: %lld", time_us_64() - expected_dtr_off_time);
    expire_time = time_us_64() + AIME_FAST_EXPIRE_US;
}

uint32_t aime_led_color()
{
    return led_color;
}
