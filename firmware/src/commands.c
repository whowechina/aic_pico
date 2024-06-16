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

static void display_nfc()
{
    printf("[NFC Module]\n");
    printf("    %s (%s)\n", nfc_module_name(), nfc_module_version());
    if (strstr(nfc_module_name(), "5180") != NULL) {
        printf("    TX Tweak: %s\n", aic_cfg->tweak.pn5180_tx ? "ON" : "OFF");
    }
}

static void display_light()
{
    printf("[Light]\n");
    printf("    RGB-%s, LED-%s\n",
            aic_cfg->light.rgb ? "ON" : "OFF",
            aic_cfg->light.led ? "ON" : "OFF");
    printf("    Level: Idle-%d, Active-%d\n", aic_cfg->light.level_idle, aic_cfg->light.level_active);
}

static void display_lcd()
{
    printf("[LCD]\n");
    printf("    Backlight: %d\n", aic_cfg->lcd.backlight);
}

static void display_reader()
{
    printf("[Reader]\n");
    printf("    Virtual AIC: %s\n", aic_cfg->reader.virtual_aic ? "ON" : "OFF");
    printf("    Mode: %s\n", mode_name(aic_cfg->reader.mode));
    if (aic_cfg->reader.mode == MODE_AUTO) {
        printf("    Detected: %s\n", mode_name(aic_runtime.mode));
    }
    if ((aic_runtime.mode == MODE_AIME0) || (aic_runtime.mode == MODE_AIME1)) {
        printf("    AIME Pattern: %s\n", aime_get_mode_string());
    }
}

static void display_warning()
{
    if (keypad_is_stuck()) {
        printf("\nWarning: Keypad disabled due to key STUCK!\n");
    }
}

static void handle_display()
{
    display_nfc();
    display_light();
    display_lcd();
    display_reader();
    display_warning();
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

    printf("Card: %s", nfc_card_name_str(card.card_type));
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
    display_reader();
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
    display_reader();
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
    display_light();
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
    display_light();
}

static void handle_lcd(int argc, char *argv[])
{
    const char *usage = "Usage: lcd <backlight>\n"
                        "    backlight: [0..255]\n";
    if (argc != 1) {
        printf(usage);
        return;
    }

    int backlight = cli_extract_non_neg_int(argv[0], 0);
    if ((backlight < 0) || (backlight > 255)) {
        printf(usage);
        return;
    }

    aic_cfg->lcd.backlight = backlight;
    config_changed();
    display_lcd();
}

static void handle_pn5180_tweak(int argc, char *argv[])
{
    const char *usage = "Usage: pn5180_tweak <on|off>\n";
    if (argc != 1) {
        printf("%s", usage);
        return;
    }

    const char *commands[] = { "on", "off" };
    int match = cli_match_prefix(commands, 4, argv[0]);
    if (match < 0) {
        return;
    }
    aic_cfg->tweak.pn5180_tx = (match == 0);
    printf("PN5180 TX Tweak: %s\n", aic_cfg->tweak.pn5180_tx ? "ON" : "OFF");
    nfc_pn5180_tx_tweak(aic_cfg->tweak.pn5180_tx);
    config_changed();
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
    cli_register("lcd", handle_lcd, "Touch LCD settings.");
    cli_register("pn5180_tweak", handle_pn5180_tweak, "PN5180 TX tweak.");
    cli_register("debug", handle_debug, "Toggle debug.");
}
