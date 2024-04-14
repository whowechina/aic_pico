/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include "config.h"
#include "save.h"

aic_cfg_t *aic_cfg;

static aic_cfg_t default_cfg = {
    .light = { .min = 24, .max = 128, .rgb = true, .led = true },
    .virtual_aic = true,
    .mode = 0,
};

aic_runtime_t *aic_runtime;

static void config_loaded()
{
    if (aic_cfg->light.min > aic_cfg->light.max) {
        aic_cfg->light = default_cfg.light;
        config_changed();
    }
}

void config_changed()
{
    save_request(false);
}

void config_factory_reset()
{
    *aic_cfg = default_cfg;
    save_request(true);
}

void config_init()
{
    aic_cfg = (aic_cfg_t *)save_alloc(sizeof(*aic_cfg), &default_cfg, config_loaded);
}
