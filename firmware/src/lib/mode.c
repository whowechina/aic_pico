/*
 * COM Port Protocol Definition and Detection
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "mode.h"

static uint64_t last_detect_time;

reader_mode_t mode_detect(const uint8_t *data, uint32_t len, uint32_t baudrate)
{
    last_detect_time = time_us_64();

    if ((len > 2) && (data[0] == 0xe0) && (data[1] < 0x10)) {
        return baudrate == 115200 ? MODE_AIME0 : MODE_AIME1;
    }

    if ((len == 1) && (data[0] == 0x55)) {
        return MODE_BANA;
    }

    if ((len >= 3) && (memcmp(data, "\x00\x00\xff", 3) == 0)) {
        return MODE_BANA;
    }
    if ((len >= 4) && (memcmp(data, "\x55\x00\x00\xff", 4) == 0)) {
        return MODE_BANA;
    }

    last_detect_time = 0;
    return MODE_NONE;
}

const char *mode_name(reader_mode_t mode)
{
    switch (mode) {
        case MODE_AUTO:
            return "Auto";
        case MODE_AIME0:
            return "Aime0";
        case MODE_AIME1:
            return "Aime1";
        case MODE_BANA:
            return "Bana";
        case MODE_NONE:
            return "None";
        default:
            return "Unknown";
    }
}