/*
 * Bandai Namco NFC Reader
 * WHowe <github.com/whowechina>
 * 
 * Use NFC Module to read BANA
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "nfc.h"
#include "bana.h"

static bool debug = false;
#define DEBUG(...) if (nfc_runtime.debug) printf(__VA_ARGS__)

#define BANA_EXPIRE_US (1200 * 1000000ULL)
#define BANA_FAST_EXPIRE_US (3 * 1000000ULL)

static void putc_trap(uint8_t byte)
{
}

static bana_putc_func bana_putc = putc_trap;

static void bana_puts(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        bana_putc(str[i]);
    }
}

void bana_init(bana_putc_func putc_func)
{
    bana_putc = putc_func;
}

typedef union __attribute__((packed)) {
    struct {
        struct {
            uint8_t padding[3];
            uint8_t len;
            uint8_t len_check;
        } hdr; 
        uint8_t dir;
        uint8_t cmd;
        uint8_t data[0];
    };
    uint8_t raw[128];
} message_t;

typedef struct __attribute__((packed)) {
    uint8_t card_present;
    uint8_t num;
    uint8_t atqa[2];
    union {
        struct {
            uint8_t sak;
            uint8_t unk;
            uint8_t uid[4];
        } mifare;
        struct {
            uint8_t idm[8];
            uint8_t pmm[8];
            uint8_t system_code[2];
        } felica;
    };
} card_report_t;

static message_t request, response;

static struct {
    uint8_t frame_len;
    uint32_t time;
} req_ctx;

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

static uint64_t expire_time;

static void send_response()
{
    uint8_t checksum = 0xff;
    for (int i = 0; i < response.hdr.len; i++) {
        checksum += response.raw[5 + i];
    }

    memcpy(response.hdr.padding, "\x00\x00\xff", 3);
    response.hdr.len_check = ~response.hdr.len + 1;

    response.raw[5 + response.hdr.len] = ~checksum;
    response.raw[6 + response.hdr.len] = 0;

    int total_len = 7 + response.hdr.len;
    bana_puts((const char *)response.raw, total_len);

    DEBUG("\n\033[33m%6ld<< %02x", time_us_32() / 1000, response.cmd);
    for (int i = 0; i < response.hdr.len - 2; i++) {
        DEBUG(" %02x", response.data[i]);
    }
    DEBUG("\033[0m");
}

static void send_response_data(const void *data, int len)
{
    response.hdr.len = 2 + len;
    response.dir = 0xd5;
    response.cmd = request.cmd + 1;
    if (len) {
        memcpy(response.data, data, len);
    }
    send_response();
}

static void send_simple_response()
{
    send_response_data(NULL, 0);
}

static void send_ack()
{
    bana_puts("\x00\x00\xff\x00\xff\x00", 6);
}

static struct {
    int led;
    int beep;
} bana_gpio;

static void cmd_gpio()
{
    if (request.data[0] == 0x01) {
        DEBUG("\nLED:%02x", request.data[1]);
        bana_gpio.led = request.data[1];
    } else if (request.data[0] == 0x08) {
        DEBUG("\nBEEP:%02x", request.data[1]);
        bana_gpio.beep = request.data[1];
    }

    send_simple_response(0x0e);
}

static void cmd_rf_field()
{
    if (memcmp(request.data, "\x01\x00", 2) == 0) {
        nfc_rf_field(false);
    } else {
        nfc_rf_field(true);
    }
    send_simple_response(request.cmd);
}

static void handle_mifare(const uint8_t uid[4])
{
    card_report_t card;

    card.card_present = 1;
    card.num = 1;
    card.atqa[0] = 0x00;
    card.atqa[1] = 0x04;
    card.mifare.sak = 0x08;
    card.mifare.unk = 0x04;
    memcpy(card.mifare.uid, uid, 4);

    send_response_data(&card, 10);
}

static void handle_felica(const uint8_t idm[8], const uint8_t pmm[8],
                          const uint8_t system_code[2])
{
    card_report_t card;
    card.card_present = 1;
    card.num = 1;
    card.atqa[0] = 0x14;
    card.atqa[1] = 0x01;
    memcpy(card.felica.idm, idm, 8);
    memcpy(card.felica.pmm, pmm, 8);
    memcpy(card.felica.system_code, system_code, 2);
    send_response_data(&card, sizeof(card));
}

static void handle_no_card()
{
    send_response_data("\x00\x00\x00", 3);
}

static void cmd_poll_card()
{
    bool mifare = (request.data[1] == 0);
    bool felica = (request.data[1] == 1);
    nfc_card_t card = nfc_detect_card_ex(mifare, felica, false);
    if (debug) {
        display_card(&card);
    }
    switch (card.card_type) {
        case NFC_CARD_MIFARE:
            handle_mifare(card.uid);
            break;
        case NFC_CARD_FELICA:
            handle_felica(card.uid, card.pmm, card.syscode);
            break;
        default:
            handle_no_card();
            break;
    }
}

static void cmd_mifare_auth(uint8_t key_id)
{
    typedef struct __attribute__((packed)) {
        uint8_t unk;
        uint8_t cmd;
        uint8_t block;
        uint8_t key[6];
        uint8_t uid[4];
    } auth_t;

    auth_t *auth = (auth_t *)request.data;

    if (nfc_mifare_auth(auth->uid, auth->block, key_id, auth->key)) {
        send_response_data("\x00", 1);
    } else {
        send_response_data("\x01", 1);
    }
}

static void cmd_mifare_read()
{
    typedef struct __attribute__((packed)) {
        uint8_t unk;
        uint8_t cmd;
        uint8_t block;
    } read_t;
    read_t *read = (read_t *)request.data;
    struct __attribute__((packed)) {
        uint8_t status;
        uint8_t data[16];
    } resp;
    if (nfc_mifare_read(read->block, resp.data)) {
        resp.status = 0;
        send_response_data(&resp, sizeof(resp));
    } else {
        send_response_data("\x14", 1);
    }
}

static void cmd_mifare()
{
    switch (request.data[1]) {
        case 0x60:
            cmd_mifare_auth(0);
            break;
        case 0x61:
            cmd_mifare_auth(1);
            break;
        case 0x30:
            cmd_mifare_read();
            break;
        default:
            DEBUG("\nUnknown mifare cmd: %02x\n", request.data[0]);
            send_ack();
            break;
    }
}

static void cmd_commthru()
{
    send_response_data("\x01", 1); // not sure if this is correct
}

static void cmd_select()
{
    nfc_select(0);
    send_response_data("\x00", 1);
    nfc_select(1);
}

static void cmd_deselect()
{
    nfc_deselect();
    send_response_data("\x01\x00", 2);
}

static void cmd_release()
{
    send_response_data("\x01\x00", 2);
}

/* https://github.com/chujohiroto/Raspberry-RCS620S/blob/master/rcs620s.py */
static void cmd_felica_read(void *read_req)
{
    typedef struct __attribute__((packed)) {
        uint8_t idm[8];
        uint8_t service_num;
        uint8_t service[2];
        uint8_t block_num;
        uint8_t block[0][2];
    } read_t;
    read_t *read = (read_t *)(request.data + 4);

    struct __attribute__((packed)) {
        uint8_t status;
        uint8_t len;
        uint8_t cmd;
        uint8_t idm[8];
        uint8_t service[2];
        uint8_t block_num;
        uint8_t block[4][16];
    } resp;

    int block_num = read->block_num;
    DEBUG("\nFelica read: ");

    block_num = (block_num > 4) ? 4: block_num;

    resp.status = 0;
    resp.len = 2 + 8 + 2 + 1 + block_num * 16;
    memset(resp.service, 0, 2);
    resp.block_num = block_num;
    resp.cmd = 0x07;
    memcpy(resp.idm, read->idm, 8);
    
    for (int i = 0; i < block_num; i++) {
        uint16_t service = read->service[0] | (read->service[1] << 8);
        uint16_t block = (read->block[i][0] << 8) | read->block[i][1];
        DEBUG("[%04x %04x]", service, block);
        if (!nfc_felica_read(service, block, resp.block[i])) {
            DEBUG(":ERR");
        }
    }

    send_response_data(&resp, 3 + 8 + 2 + 1 + block_num * 16);
}

static void cmd_felica()
{
    typedef struct __attribute__((packed)) {
        uint16_t timeout;
        uint8_t len;
        uint8_t cmd;
        uint8_t data[0];
    } felica_t;
    felica_t *felica = (felica_t *)request.data;
    if ((felica->cmd == 0x06) && (felica->len = request.hdr.len - 2)) {
        cmd_felica_read(felica->data);
    } else {
        DEBUG("\nBad felica cmd: %02x %d", felica->cmd, felica->len);
    }
}

static void handle_frame()
{
    switch (request.cmd) {
        case 0x18:
        case 0x12:
            send_simple_response(request.cmd);
            break;
        case 0x0e:
            cmd_gpio();
            break;
        case 0x08:
            nfc_rf_field(false);
            send_response_data("\0", 1);
            break;
        case 0x06:
            if (request.data[1] == 0x1c) {
                send_response_data("\xff\x3f\x0e\xf1\xff\x3f\x0e\xf1", 8);
            } else {
                send_response_data("\xdc\xf4\x3f\x11\x4d\x85\x61\xf1\x26\x6a\x87", 11);
            }
            break;
        case 0x32:
            cmd_rf_field();
            break;
        case 0x0c:
            send_response_data("\x00\x06\x00", 3);
            break;
        case 0x4a:
            cmd_poll_card();
            break;
        case 0x40:
            cmd_mifare();
            break;
        case 0x42:
            cmd_commthru();
            break;
        case 0x44:
            cmd_deselect();
            break;
        case 0xa0:
            cmd_felica();
            break;
        case 0x52:
            cmd_release();
            break;
        case 0x54:
            cmd_select();
            break;
        default:
            printf("\nUnknown cmd: %02x (%d)\n", request.cmd, request.hdr.len);
            send_ack();
            break;
    }
}

bool bana_feed(int c)
{
    uint32_t now = time_us_32();

    if ((req_ctx.frame_len == sizeof(request)) ||
        (now - req_ctx.time > 100000))  {
        req_ctx.frame_len = 0;
    }

    req_ctx.time = now;

    request.raw[req_ctx.frame_len] = c;
    req_ctx.frame_len++;

    if ((req_ctx.frame_len == 1) && (request.raw[0] == 0x55)) {
        req_ctx.frame_len = 0;
    } if ((req_ctx.frame_len == 3) &&
        (memcmp(request.hdr.padding, "\x00\x00\xff", 3) != 0)) {
        request.raw[0] = request.raw[1];
        request.raw[1] = request.raw[2];
        req_ctx.frame_len--;
    } if ((req_ctx.frame_len == 6) && (request.hdr.len == 0)) {
        req_ctx.frame_len = 0;
    } else if (req_ctx.frame_len == request.hdr.len + 7) {
        handle_frame();
        req_ctx.frame_len = 0;
        expire_time = time_us_64() + BANA_EXPIRE_US;
    }
    return true;
}

bool bana_is_active()
{
    return time_us_64() < expire_time;
}

void bana_dtr_off()
{
    if (!bana_is_active()) {
        return;
    }
    expire_time = time_us_64() + BANA_FAST_EXPIRE_US;
}

static const struct {
    int cmd;
    const char *pattern;
} bana_led_patterns[] = {
    { 0x00, " 1, #000000, 0" }, // off
    { 0x01, " 1, #0000ff, 50" }, // blue
    { 0x02, " 1, #ff0000, 50" }, // red
    { 0x03, " 1, #00ff00, 50" }, // green
    { 0x04, "-1, #0000ff, 100, #000000, 100" }, // fast blue flash
    { 0x05, "-1, #0000ff, 500, #000000, 500" }, // slow blue flash
    { 0x06, "-1, #0000ff, 200, #000000, 200" }, // regular blue flash
    { 0x07, "-1, #0000ff, 200, #000000, 0, #000000, 1000" }, // blue flash with pause
    { 0x08, "-1, #ffff00, 200, #000000, 200, #ff0000, 200, #000000, 200" }, // yellow and red cycle
    { 0x09, "-1, #ff0000, 200, #000000, 200" }, // red on off flashing
    { 0x0a, " 1, #ff0000, 300, #00ff00, 300, #0000ff, 300" }, // rgb cycle once
    { 0x0b, "-1, #ff0000, 300, #00ff00, 300, #0000ff, 300" }, // rgb cycle endless
    { 0x0c, "-1, #00ff00, 100, #0000ff, 100" }, // green blue epilepsy
    { 0x0d, "-1, #00ff00, 100, #0000ff, 100" }, // green blue quick softer
    { 0x0e, "-1, #ffffff, 300, #ff00ff, 300, #00ffff, 300" }, // white pink cyan
    { 0x0f, "-1, #ff0000, 100, #00ff00, 100, #0000ff, 100" }, // rgb something
    { 0x11, " 1, #00ff00, 200, #007f00, 200, #00ff00, 200, #007f00, 200, #0000ff, 200" }, // green to blue
    { 0x14, " 1, #00ff00, 200, #00ff00, 1000, #000000, 0" }, // green then off
    { 0x16, " 1, #ff0000, 200, #0000ff, 200" }, // red to blue
    { 0x19, " 1, #ff0000, 200, #ff0000, 1000, #000000, 0" }, // red then off
    { 0x1b, " 1, #0000ff, 200" } // to blue
};

const char *bana_get_led_pattern()
{
    if (bana_gpio.led < 0) {
        return NULL;
    }

    int cmd = bana_gpio.led;
    bana_gpio.led = -1;

    int patt_num = sizeof(bana_led_patterns) / sizeof(bana_led_patterns[0]);
    for (int i = 0; i < patt_num; i++) {
        if (bana_led_patterns[i].cmd == cmd) {
            return bana_led_patterns[i].pattern;
        }
    }

    return NULL;
}
