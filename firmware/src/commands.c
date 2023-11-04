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

#include "pn532.h"

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
    printf("[Config]\n");
    printf("    LED level: %d\n", aic_cfg->led.level);
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
    bool ret;

//    ret = pn532_config_rf();
//    printf("RF: %d\n", ret);

    ret = pn532_config_sam();
    printf("Sam: %d\n", ret);

    uint8_t buf[32];
    int len = sizeof(buf);

    ret = pn532_poll_mifare(buf, &len);
    printf("Mifare: %d -", len);

    if (ret) {
        for (int i = 0; i < len; i++) {
            printf(" %02x", buf[i]);
        }
    }
    printf("\n");

    printf("Felica: ");
    if (pn532_poll_felica(buf, buf + 8, buf + 16, false)) {
        for (int i = 0; i < 18; i++) {
            printf(" %02x%s", buf[i], (i % 8 == 7) ? "," : "");
        }
    }
    printf("\n");
}

static void handle_level(int argc, char *argv[])
{
    const char *usage = "Usage: level <0..255>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }

    int level = cli_extract_non_neg_int(argv[0], 0);
    if ((level < 0) || (level > 255)) {
        printf(usage);
        return;
    }

    aic_cfg->led.level = level;
    config_changed();
    handle_display();
}

void commands_init()
{
    cli_register("display", handle_display, "Display all settings.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
    cli_register("nfc", handle_nfc, "NFC debug.");
    cli_register("level", handle_level, "Set LED level.");
}
