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

#include "cardio.h"

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

static void disp_list()
{
    for (int i = 0; i < 4; i++) {
        printf("    SLOT %d:", i);
        if (aic_cfg->autopin.entries[i].uidlen == 0) {
            printf(" Empty\n");
            continue;
        }
        printf(" UID: ");
        for (int j = 0; j < aic_cfg->autopin.entries[i].uidlen; j++) {
            printf("%02x", aic_cfg->autopin.entries[i].uid[j]);
        }
        printf(" PIN: %.15s", aic_cfg->autopin.entries[i].pin);
        if (aic_cfg->autopin.entries[i].swipe) {
            printf(" [Swipe]");
        }
        if (aic_cfg->autopin.entries[i].delay < 100) {
            printf(" [Delay: %ds]", aic_cfg->autopin.entries[i].delay);
        }
        printf("\n");
    }
}

static void display_autopin()
{
    printf("[AUTO PIN-Entry]\n");
    printf("    Status: %s\n", aic_cfg->autopin.enabled ? "ON" : "OFF");
    disp_list();
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
    display_autopin();
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

static void autopin_onoff(bool on)
{
    aic_cfg->autopin.enabled = on;
    config_changed();
    printf("Auto Pin-entry: %s\n", on ? "ON" : "OFF");
}

static void autopin_delete(int argc, char *argv[])
{
    const char *usage = "Usage: autopin delete <SLOT>\n"
                        "  SLOT: [0..3], all";
    if (argc != 1) {
        printf("%s", usage);
        return;
    }

    const char *slots[] = { "0", "1", "2", "3", "all" };
    int match = cli_match_prefix(slots, 5, argv[0]);
    if (match < 0) {
        printf("%s", usage);
        return;
    }
    if (match == 4) {
        memset(&aic_cfg->autopin.entries, 0, sizeof(aic_cfg->autopin.entries));
        printf("All slots cleared.\n");
    } else {
        memset(&aic_cfg->autopin.entries[match], 0,
               sizeof(aic_cfg->autopin.entries[match]));
        printf("Slot %d cleared.\n", match);
    }
    config_changed();
}

static int find_slot(const uint8_t uid[8], uint8_t uidlen)
{
    int slot = -1;
    for (int i = 0; i < 4; i++) {
        if ((aic_cfg->autopin.entries[i].uidlen == uidlen) &&
            (memcmp(aic_cfg->autopin.entries[i].uid, uid, uidlen) == 0)) {
            return i;
        }
        if ((slot < 0) && (aic_cfg->autopin.entries[i].uidlen == 0)) {
            slot = i;
        }
    }
    return slot;
}

static void autopin_add(int argc, char *argv[])
{
    const char *usage = "Usage: autopin add <PIN> swipe\n"
                        "       autopin add <PIN> <delay|both> [DELAY]\n"
                        "  PIN: 4 digits\n"
                        "  DELAY: [0..99] seconds\n"
                        "\"swipe\": swipe-triggered PIN-entry (not the first swipe);\n"
                        "\"delay\": delayed auto PIN-entry;\n"
                        "\"both\": both two methods.\n";
    if (argc < 2) {
        printf("%s", usage);
        return;
    }

    uint8_t uid[8];
    uint8_t uidlen;    
    if (!cardio_get_last(uid, &uidlen)) {
        printf("No card detected.\n");
        return;
    }

    int slot = find_slot(uid, uidlen);
    if (slot < 0) {
        printf("No empty slots available.\n");
        return;
    }

    if (strlen(argv[0]) != 4) {
        printf("PIN must be 4 digits.\n");
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (!isdigit((unsigned char)argv[0][i])) {
            printf("PIN must be 4 digits.\n");
            return;
        }
    }

    const char *commands[] = { "swipe", "delay", "both" };
    int match = cli_match_prefix(commands, count_of(commands), argv[1]);
    if (match < 0) {
        printf("%s", usage);
        return;
    }
    if (match == 0) {
        if (argc != 2) {
            printf("%s", usage);
            return;
        }
        aic_cfg->autopin.entries[slot].delay = 127; // disable delayed entry
        aic_cfg->autopin.entries[slot].swipe = true;
    } else {
        if (argc != 3) {
            printf("%s", usage);
            return;
        }
        int delay = cli_extract_non_neg_int(argv[2], 0);
        if ((delay < 0) || (delay > 99)) {
            printf("%s", usage);
            return;
        }
        aic_cfg->autopin.entries[slot].swipe = (match == 2);
        aic_cfg->autopin.entries[slot].delay = delay;
    }

    aic_cfg->autopin.entries[slot].uidlen = uidlen;
    memcpy(aic_cfg->autopin.entries[slot].uid, uid, uidlen);
    strcpy(aic_cfg->autopin.entries[slot].pin, argv[0]);

    config_changed();
    printf("Added to slot %d.\n", slot);
    disp_list();
}

static void handle_autopin(int argc, char *argv[])
{
    const char *usage = "Usage: autopin <on|off>\n"
                        "       autopin list\n"
                        "       autopin delete <SLOT>\n"
                        "       autopin add <PIN> swipe\n"
                        "       autopin add <PIN> <delay|both> [DELAY]\n";
    if (argc < 1) {
        printf("%s", usage);
        return;
    }

    const char *commands[] = { "on", "off", "list", "delete", "add" };
    int match = cli_match_prefix(commands, 5, argv[0]);
    if ((match == 0) || (match == 1)) {
        if (argc == 1) {
            autopin_onoff(match == 0);
            return;
        }
    } else if (match == 2) {
        if (argc == 1) {
            disp_list();
            return;
        }
    } else if (match == 3) {
        autopin_delete(argc - 1, argv + 1);
        return;
    } else if (match == 4) {
        autopin_add(argc - 1, argv + 1);
        return;
    }

    printf("%s", usage);
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
    cli_register("autopin", handle_autopin, "Auto pin-entry.");
    cli_register("pn5180_tweak", handle_pn5180_tweak, "PN5180 TX tweak.");
    cli_register("debug", handle_debug, "Toggle debug.");
}
