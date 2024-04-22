/*
 * Bandai Namco NFC Reader
 * WHowe <github.com/whowechina>
 * 
 * Use NFC Module to read BANA
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bsp/board.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "nfc.h"
#include "bana.h"

static bool debug = false;
#define DEBUG(...) if (debug) printf(__VA_ARGS__)

#define BANA_EXPIRE_TIME 10000000ULL

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

void bana_debug(bool enable)
{
    debug = enable;
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
    uint8_t raw[48];
} message_t;

static message_t request, response;

struct {
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

    DEBUG("\n\033[33mResp: %2d %2x >>", response.hdr.len, response.cmd);
    for (int i = 0; i < total_len; i++) {
        DEBUG(" %02x", response.raw[i]);
    }
    DEBUG("\033[0m");
}

static void send_simple_response(uint8_t code)
{
    response.hdr.len = 2;
    response.dir = 0xd5;
    response.cmd = code;
    send_response();
}

static void send_ack()
{
    bana_puts("\x00\x00\xff\x00\xff\x00", 6);
}

static uint32_t led_color = 0;

static void handle_frame()
{
    DEBUG("\nBana >>");

    for (int i = 3; i < request.hdr.len; i++) {
        DEBUG(" %02x", request.data[i]);
    }

    if (request.hdr.len != 0) {
        send_ack();
        send_simple_response(request.cmd + 1);
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
        expire_time = time_us_64() + BANA_EXPIRE_TIME;
    }
    return true;
}

bool bana_is_active()
{
    return time_us_64() < expire_time;
}

uint32_t bana_led_color()
{
    return led_color;
}
