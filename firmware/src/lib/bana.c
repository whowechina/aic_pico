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

#define BANA_EXPIRE_TIME 5000000ULL

static void putc_trap(uint8_t byte)
{
}

static bana_putc_func bana_putc = putc_trap;

void bana_init(bana_putc_func putc_func)
{
    bana_putc = putc_func;
}

/*
static uint8_t mifare_keys[2][6]; // 'KeyA' and 'KeyB'

static union __attribute__((packed)) {
    struct {
    };
    uint8_t raw[256];
} response;

static union __attribute__((packed)) {
    struct {
    };
    uint8_t raw[256];
} request;

struct {
    bool active;
    uint8_t len;
    uint8_t check_sum;
    bool escaping;
    uint64_t time;
} req_ctx;
*/

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

bool bana_feed(int c)
{
    return true;
}

uint64_t bana_expire_time()
{
    return expire_time;
}
