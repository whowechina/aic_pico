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
#include "pn5180.h"

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
    printf("    Light: RGB-%s LED-%s\n",
            aic_cfg->light.rgb ? "ON" : "OFF",
            aic_cfg->light.led ? "ON" : "OFF");
    printf("    Level: [%d ~ %d]\n", aic_cfg->light.min, aic_cfg->light.max);
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
    const char *usage = "Usage: level <0..255> <0..255>\n";
    if (argc != 2) {
        printf(usage);
        return;
    }

    int min = cli_extract_non_neg_int(argv[0], 0);
    if ((min < 0) || (min > 255)) {
        printf(usage);
        return;
    }
    int max = cli_extract_non_neg_int(argv[1], 0);
    if (max > 255) {
        printf(usage);
        return;
    }

    if (max < min) {
        max = min;
    }

    aic_cfg->light.min = min;
    aic_cfg->light.max = max;

    config_changed();
    handle_display();
}

static void handle_pnboot()
{
    pn5180_reset();
}

static void handle_pnver()
{
    uint8_t buf[6];
    pn5180_read_eeprom(0x10, buf, sizeof(buf));

    printf("Version: %02x %02x %02x %02x %02x %02x\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    pn5180_print_rf_cfg();
}

static void handle_pnread(int argc, char *argv[])
{
    int reg = cli_extract_non_neg_int(argv[0], 0);
    printf("%2d: %08lx\n", reg, pn5180_read_reg(reg));
}

static void handle_pnmifare()
{
    pn5180_load_rf_config(0x00, 0x80); // 1
    pn5180_rf_on(); // 2

    sleep_ms(2000);

    pn5180_and_reg(PN5180_REG_CRC_TX_CONFIG, 0xfffffffe); // 3
    pn5180_and_reg(PN5180_REG_CRC_RX_CONFIG, 0xfffffffe); // 4

    pn5180_and_reg(PN5180_REG_IRQ_CLEAR, 0x000fffff); // 5
    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8); // 6
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03); // 7

    uint8_t buf[] = {0x26};
    pn5180_send_data(buf, sizeof(buf), 7); // 8

    for (int i = 0; i < 10; i++) {
        printf("irq: %08lx rx: %08lx\n", pn5180_read_reg(PN5180_REG_IRQ_STATUS),
                                       pn5180_read_reg(PN5180_REG_RX_STATUS));
        sleep_ms(2);
    }

    uint8_t out[128] = {0};
    pn5180_read_data(out, 2); // 10

    printf("2: %02x %02x\n", out[0], out[1]);

#if 0
    uint8_t anti_collision[] = {0x93, 0x20};
    pn5180_send_data(anti_collision, sizeof(anti_collision), 0);

    pn5180_read_data(out + 2, 5);
    printf("5: %02x %02x %02x %02x %02x\n", out[2], out[3], out[4], out[5], out[6]);

    pn5180_or_reg(PN5180_REG_CRC_RX_CONFIG, 0x01);
    pn5180_or_reg(PN5180_REG_CRC_TX_CONFIG, 0x01);

    anti_collision[1] = 0x70;
    pn5180_send_data(anti_collision, sizeof(anti_collision), 0);
    pn5180_read_data(out + 7, 1); // sak
    printf("1: %02x\n", out[7]);
#endif
    pn5180_rf_off();
}

static void handle_pnfeli()
{
    pn5180_load_rf_config(0x09, 0x89);
    pn5180_rf_on();

    sleep_ms(1000);

    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xffffffbf);

    uint8_t cmd[] = {0x06, 0x00, 0xff, 0xff, 0x01, 0x00};

	pn5180_send_data(cmd, 6, 0x00);

    sleep_ms(100);

    uint8_t out[32] = {0};
    pn5180_read_data(out, 20);

    pn5180_rf_off();

    printf("feli:");
    for (int i = 0; i < 20; i++) {
        printf(" %02x", out[i]);
    }
    printf("\n");
}


static void handle_pninv()
{
    pn5180_load_rf_config(0x0d, 0x8d);
    pn5180_rf_on();

    sleep_ms(1000);

    pn5180_and_reg(PN5180_REG_IRQ_CLEAR, 0x000fffff); // 5
    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8); // 6
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03); // 7

    uint8_t cmd[] = {0x06, 0x01, 0x00};
    pn5180_send_data(cmd, 3, 0);

    sleep_ms(10);

    for (int i = 0; i < 10; i++) {
        printf("irq: %08lx rx: %08lx\n", pn5180_read_reg(PN5180_REG_IRQ_STATUS),
                                       pn5180_read_reg(PN5180_REG_RX_STATUS));
        sleep_ms(1);
    }


    uint32_t rxstatus = pn5180_read_reg(PN5180_REG_RX_STATUS);
    int len = rxstatus & 0x1ff;
    uint8_t buf[len];
    pn5180_read_data(buf, len);

    printf("uid:");
    for (int i = 0; i < len; i++) {
        printf(" %02x", buf[i]);
    }
    printf("\n");
    pn5180_rf_off();
}

void commands_init()
{
    cli_register("display", handle_display, "Display all settings.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
    cli_register("nfc", handle_nfc, "NFC debug.");
    cli_register("light", handle_light, "Turn on/off lights.");
    cli_register("level", handle_level, "Set light level.");
    cli_register("pnboot", handle_pnboot, "PN5180 reboot");
    cli_register("pnver", handle_pnver, "PN5180 version");
    cli_register("pnread", handle_pnread, "PN5180 debug rf");
    cli_register("pnmifare", handle_pnmifare, "PN5180 mifare");
    cli_register("pnfeli", handle_pnfeli, "PN5180 felica");
    cli_register("pninv", handle_pninv, "PN5180 15693");
}
