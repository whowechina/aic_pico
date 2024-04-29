/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    struct {
        uint8_t min;
        uint8_t max;
        bool rgb;
        bool led;
    } light;
    bool virtual_aic;
    uint8_t mode;
    uint32_t reserved;
} aic_cfg_t;

typedef volatile struct {
    bool debug;
} aic_runtime_t;

extern aic_cfg_t *aic_cfg;
extern aic_runtime_t aic_runtime;

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
