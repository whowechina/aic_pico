/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include "config.h"
#include "save.h"
#include "mode.h"

aic_cfg_t *aic_cfg;

static aic_cfg_t default_cfg = {
    .light = { .level_idle = 24, .level_active = 128, .rgb = true, .led = true },
    .reader = { .virtual_aic = true, .mode = MODE_AUTO },
    .lcd = { .backlight = 200, },
    .tweak = { .pn5180_tx = false },
};

aic_runtime_t aic_runtime;

static void config_loaded()
{
    if ((aic_cfg->reader.mode != MODE_AIME0) &&
        (aic_cfg->reader.mode != MODE_AIME1) &&
        (aic_cfg->reader.mode != MODE_BANA)) {
        aic_cfg->reader.mode = MODE_AUTO;
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
