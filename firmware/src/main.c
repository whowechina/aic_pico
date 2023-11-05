/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "board_defs.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"
#include "rgb.h"

#include "pn532.h"
#include "aime.h"

static struct {
    uint8_t current[9];
    uint8_t reported[9];
    uint64_t report_time;
} cardio;

void report_usb_hid()
{
    if (!tud_hid_ready()) {
        return;
    }

    uint64_t now = time_us_64();

    if (memcmp(cardio.current, "\0\0\0\0\0\0\0\0\0", 9) != 0) {
        rgb_set_rainbow_speed(255);
    }

    if ((memcmp(cardio.current, cardio.reported, 9) != 0) &&
        (now - cardio.report_time > 1000000)) {

        tud_hid_n_report(0x00, cardio.current[0], cardio.current + 1, 8);
        memcpy(cardio.reported, cardio.current, 9);
        cardio.report_time = now;

        if (memcmp(cardio.current, "\0\0\0\0\0\0\0\0\0", 9) != 0) {
            printf("Card:");
            for (int i = 0; i < 9; i++) {
                printf(" %02x", cardio.current[i]);
            }
            printf("\n");
        }
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            mutex_exit(&core1_io_lock);
        }
        rgb_update();
        cli_fps_count(1);
        sleep_ms(1);
    }
}

void detect_card()
{
    pn532_config_sam();

    uint8_t id[20] = { 0 };

    int len = sizeof(id);
    bool mifare = pn532_poll_mifare(id, &len);
    if (mifare) {
        cardio.current[0] = REPORT_ID_EAMU;
        cardio.current[1] = 0xe0;
        cardio.current[2] = 0x04;
        if (len == 4) {
            memcpy(cardio.current + 3, id, 4);
            memcpy(cardio.current + 7, id, 2);
        } else if (len == 7) {
            memcpy(cardio.current + 3, id + 1, 6);
        }
        return;
    }

    bool felica = pn532_poll_felica(id, id + 8, id + 16, false);
    if (felica) {
        cardio.current[0] = REPORT_ID_FELICA;
        memcpy(cardio.current + 1, id, 8);
        return;
    }

    memset(cardio.current, 0, 9);
}

const int aime_intf = 1;
static void cdc_aime_putc(uint8_t byte)
{
    tud_cdc_n_write(aime_intf, &byte, 1);
    tud_cdc_n_write_flush(aime_intf);
}

static void aime_run()
{
    if (tud_cdc_n_available(aime_intf)) {
        uint8_t buf[32];
        int count = tud_cdc_n_read(aime_intf, buf, sizeof(buf));
        for (int i = 0; i < count; i++) {
            aime_feed(buf[i]);
        }
    }
}

void wait_loop()
{
    tud_task();
    cli_run();
    cli_fps_count(0);
}

static void core0_loop()
{
    while(1) {
        tud_task();

        cli_run();
        aime_run();
    
        save_loop();
        cli_fps_count(0);

        detect_card();
        report_usb_hid();
        sleep_ms(1);
    }
}

void init()
{
    sleep_ms(50);
    set_sys_clock_khz(150000, true);
    board_init();
    tusb_init();
    stdio_init_all();
    rgb_init();

    config_init();
    mutex_init(&core1_io_lock);
    save_init(0xca340a1c, &core1_io_lock);


    pn532_init(I2C_PORT, I2C_SCL, I2C_SDA, I2C_FREQ);
    pn532_set_wait_loop(wait_loop);
    aime_init(cdc_aime_putc);

    cli_init("aic_pico>", "\n     << AIC Pico >>\n"
                            " https://github.com/whowechina\n\n");
    
    commands_init();
}

int main(void)
{
    init();
    multicore_launch_core1(core1_loop);
    core0_loop();
    return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    printf("Get from USB %d-%d\n", report_id, report_type);
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
}
