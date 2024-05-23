#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "save.h"
#include "cli.h"

#include "keypad.h"

#include "aime.h"
#include "bana.h"
#include "nfc.h"

static int fps[2];
void fps_count(int core)
{
    static uint32_t last[2] = {0};
    static int counter[2] = {0};

    counter[core]++;

    uint32_t now = time_us_32();
    if (now - last[core] < 1000000) {
        return;
    }
    last[core] = now;
    fps[core] = counter[core];
    counter[core] = 0;
}

static void handle_display()
{
    printf("[NFC Module]\n");
    printf("    %s (%s)\n", nfc_module_name(), nfc_module_version());

    printf("[Light]\n");
    printf("    RGB-%s, LED-%s\n",
            aic_cfg->light.rgb ? "ON" : "OFF",
            aic_cfg->light.led ? "ON" : "OFF");
    printf("    Level: Idle-%d, Active-%d\n", aic_cfg->light.level_idle, aic_cfg->light.level_active);

    printf("[Reader]\n");
    printf("    Virtual AIC: %s\n", aic_cfg->reader.virtual_aic ? "ON" : "OFF");
    printf("    Mode: %s\n", mode_name(aic_cfg->reader.mode));
    if (aic_cfg->reader.mode == MODE_AUTO) {
        printf("    Detected: %s\n", mode_name(aic_runtime.mode));
    }
    if ((aic_runtime.mode == MODE_AIME0) || (aic_runtime.mode == MODE_AIME1)) {
        printf("    AIME Pattern: %s\n", aime_get_mode_string());
    }

    if (keypad_is_stuck()) {
        printf("\nWarning: Keypad disabled due to key STUCK!\n");
    }
}

static void handle_save()
{
    save_request(true);
}

static void handle_factory_reset()
{
    config_factory_reset();
    printf("Factory reset done.\n");
}

static void handle_nfc()
{
    printf("NFC module: %s\n", nfc_module_name());

    nfc_rf_field(true);
    nfc_card_t card = nfc_detect_card();
    nfc_rf_field(false);

    printf("Card: %s", nfc_card_name(card.card_type));
    for (int i = 0; i < card.len; i++) {
        printf(" %02x", card.uid[i]);
    }
    printf("\n");
}

static void handle_virtual(int argc, char *argv[])
{
    const char *usage = "Usage: virtual <on|off>\n";
    if (argc != 1) {
        printf("%s", usage);
        return;
    }

    const char *commands[] = { "on", "off" };
    int match = cli_match_prefix(commands, 2, argv[0]);
    if (match < 0) {
        printf("%s", usage);
        return;
    }

    aic_cfg->reader.virtual_aic = (match == 0);

    aime_virtual_aic(aic_cfg->reader.virtual_aic);
    config_changed();
}

static void handle_mode(int argc, char *argv[])
{
    const char *usage = "Usage: mode <auto|aime0|aime1|bana>\n"
                        "    auto: Auto detect\n"
                        "    aime0: Sega Aime 0\n"
                        "    aime1: Sega Aime 1\n"
                        "    bana: Bandai Namco\n";
    if (argc != 1) {
        printf("%s", usage);
        return;
    }

    reader_mode_t newmode = MODE_NONE;
    const char *commands[] = { "auto", "aime0", "aime1", "bana" };
    int match = cli_match_prefix(commands, 4, argv[0]);
    switch (match) {
        case 0:
            newmode = MODE_AUTO;
            break;
        case 1:
            newmode = MODE_AIME0;
            break;
        case 2:
            newmode = MODE_AIME1;
            break;
        case 3:
            newmode = MODE_BANA;
            break;
        default:
            printf("%s", usage);
            return;
    }

    aic_cfg->reader.mode = newmode;
    aic_runtime.mode = (newmode == MODE_AUTO) ? MODE_NONE : newmode;
    config_changed();
}

static void handle_light(int argc, char *argv[])
{
    const char *usage = "Usage: light <rgb|led|both|off>\n";
    if (argc != 1) {
        printf("%s", usage);
        return;
    }

    const char *commands[] = { "rgb", "led", "both", "off" };
    int match = cli_match_prefix(commands, 4, argv[0]);
    switch (match) {
        case 0:
            aic_cfg->light.rgb = true;
            aic_cfg->light.led = false;
            break;
        case 1:
            aic_cfg->light.rgb = false;
            aic_cfg->light.led = true;
            break;
        case 2:
            aic_cfg->light.rgb = true;
            aic_cfg->light.led = true;
            break;
        case 3:
            aic_cfg->light.rgb = false;
            aic_cfg->light.led = false;
            break;
        default:
            printf("%s", usage);
            return;
    }
    config_changed();
}

static void handle_level(int argc, char *argv[])
{
    const char *usage = "Usage: level <dimmed> <active>\n"
                        "    dimmed, active: [0..255]\n";
    if (argc != 2) {
        printf(usage);
        return;
    }

    int idle = cli_extract_non_neg_int(argv[0], 0);
    int active = cli_extract_non_neg_int(argv[1], 0);
    if ((idle < 0) || (idle > 255) ||
        (active < 0) || (active > 255)) {
        printf(usage);
        return;
    }

    aic_cfg->light.level_idle = idle;
    aic_cfg->light.level_active = active;

    config_changed();
    handle_display();
}

static void handle_debug()
{
    aic_runtime.debug = !aic_runtime.debug;
    nfc_runtime.debug = aic_runtime.debug;
    printf("Debug: %s\n", aic_runtime.debug ? "ON" : "OFF");
}

void commands_init()
{
    cli_register("display", handle_display, "Display all settings.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
    cli_register("nfc", handle_nfc, "NFC module.");
    cli_register("virtual", handle_virtual, "Virtual AIC card.");
    cli_register("mode", handle_mode, "Reader mode/protocol.");
    cli_register("light", handle_light, "Turn on/off lights.");
    cli_register("level", handle_level, "Set light level.");
    cli_register("debug", handle_debug, "Toggle debug.");
}
