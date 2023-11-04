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

    bool report = false;

    if (memcmp(cardio.current, cardio.reported, 9) != 0) {
        const uint8_t empty[9] = { 0 };
        if ((memcmp(cardio.current, empty, 9) != 0) ||
            (now - cardio.report_time > 5000000)) {
            /* Immediate report for new card,
               5 seconds delay for for card removal */
            report = true;
        }
    } else if (now - cardio.report_time > 1000000) {
        /* Same card, reports every 0.5 sec */
        report = true;
    }

    if (report) {
        tud_hid_n_report(0x00, cardio.current[0], cardio.current + 1, 8);
        memcpy(cardio.reported, cardio.current, 9);
        cardio.report_time = now;

        const uint8_t empty[9] = { 0 };
        if (memcmp(cardio.current, empty, 9) == 0) {
            return;
        }

        printf("Card:");
        for (int i = 0; i < 9; i++) {
            printf(" %02x", cardio.current[i]);
        }
        printf("\n");
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            mutex_exit(&core1_io_lock);
        }
        rgb_set_color(0, 0x800000);
        rgb_set_color(1, 0x008000);
        rgb_set_color(2, 0x000080);
        rgb_set_color(3, 0x808080);
        rgb_update();
        cli_fps_count(1);
        sleep_ms(1);
    }
}

void detect_card()
{
    static bool poll_mifare = false;
    static bool detected_mifare = false;

    if (poll_mifare) {
        pn532_config_sam();
        uint8_t id[8] = { 0 };
        int len = sizeof(id);
        detected_mifare = pn532_poll_mifare(id, &len);
        if (detected_mifare) {
            cardio.current[0] = REPORT_ID_EAMU;
            cardio.current[1] = 0xe0;
            cardio.current[2] = 0x04;
            if (len == 4) {
                memcpy(cardio.current + 3, id, 4);
                memcpy(cardio.current + 7, id, 2);
            } else if (len == 7) {
                memcpy(cardio.current + 3, id + 1, 6);
            } else {
                detected_mifare = false;
            }
        }
        poll_mifare = false;
    } else {
        pn532_config_sam();
        uint8_t id[18] = { 0 };
        //bool detected_felica = pn532_poll_felica(id, id + 8, id + 16, false);
        bool detected_felica = false;
        if (detected_felica) {
            cardio.current[0] = REPORT_ID_FELICA;
            memcpy(cardio.current + 1, id, 8);
        } else if (!detected_mifare) {
            memset(cardio.current, 0, 9);
        }
        poll_mifare = true;
    }
}

void wait_loop()
{
    tud_task();

    cli_run();
    aime_update();

    cli_fps_count(0);
}

static void core0_loop()
{
    while(1) {
        tud_task();

        cli_run();
        aime_update();
    
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
    aime_init(1);

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
