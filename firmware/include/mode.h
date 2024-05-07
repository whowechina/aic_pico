/*
 * COM Port Protocol Definition and Detection
 * WHowe <github.com/whowechina>
 */

#ifndef MODE_H
#define MODE_H

#include <stdint.h>

typedef enum {
    MODE_AUTO = 0,
    MODE_AIME0 = 0x10,
    MODE_AIME1 = 0x11,
    MODE_BANA = 0x20,
    MODE_NONE = 0xff,
} reader_mode_t;

reader_mode_t mode_detect(const uint8_t *data, uint32_t len, uint32_t baudrate);
const char *mode_name(reader_mode_t mode);

#endif
