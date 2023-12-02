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
#include "light.h"
#include "keypad.h"

#include "pn532.h"
#include "pn5180.h"

#include "aime.h"

static struct {
    uint8_t current[9];
    uint8_t reported[9];
    uint64_t report_time;
} hid_cardio;

void report_hid_cardio()
{
    if (!tud_hid_ready()) {
        return;
    }

    uint64_t now = time_us_64();

    if (memcmp(hid_cardio.current, "\0\0\0\0\0\0\0\0\0", 9) != 0) {
        light_stimulate();
    }

    if ((memcmp(hid_cardio.current, hid_cardio.reported, 9) != 0) &&
        (now - hid_cardio.report_time > 1000000)) {

        tud_hid_n_report(0x00, hid_cardio.current[0], hid_cardio.current + 1, 8);
        memcpy(hid_cardio.reported, hid_cardio.current, 9);
        hid_cardio.report_time = now;

        if (memcmp(hid_cardio.current, "\0\0\0\0\0\0\0\0\0", 9) != 0) {
            printf("Card:");
            for (int i = 0; i < 9; i++) {
                printf(" %02x", hid_cardio.current[i]);
            }
            printf("\n");
        }
    }
}

struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t keymap[15];
} hid_nkro;

static const char keymap[12] = KEYPAD_NKRO_MAP;

void report_hid_key()
{
    if (!tud_hid_ready()) {
        return;
    }

    uint16_t keys = keypad_read();
    for (int i = 0; i < keypad_key_num(); i++) {
        uint8_t code = keymap[i];
        uint8_t byte = code / 8;
        uint8_t bit = code % 8;
        if (keys & (1 << i)) {
            hid_nkro.keymap[byte] |= (1 << bit);
        } else {
            hid_nkro.keymap[byte] &= ~(1 << bit);
        }
    }
    tud_hid_n_report(1, 0, &hid_nkro, sizeof(hid_nkro));
}

void report_usb_hid()
{
    report_hid_cardio();
    report_hid_key();
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            light_update();
            mutex_exit(&core1_io_lock);
        }
        cli_fps_count(1);
        sleep_ms(1);
    }
}

static enum {
    NFC_UNKNOWN,
    NFC_PN532,
    NFC_PN5180
} nfc_module = NFC_PN5180;

void detect_card()
{
    if (nfc_module == NFC_PN532) {
        pn532_config_sam();
    }

    uint8_t id[20] = { 0 };

    int len = sizeof(id);
    bool mifare = false;
    if (nfc_module == NFC_PN532) {
        mifare = pn532_poll_mifare(id, &len);
    } else if (nfc_module == NFC_PN5180) {
        mifare = pn5180_poll_mifare(id, &len);
    }
    if (mifare) {
        hid_cardio.current[0] = REPORT_ID_EAMU;
        hid_cardio.current[1] = 0xe0;
        hid_cardio.current[2] = 0x04;
        if (len == 4) {
            memcpy(hid_cardio.current + 3, id, 4);
            memcpy(hid_cardio.current + 7, id, 2);
        } else if (len == 7) {
            memcpy(hid_cardio.current + 3, id + 1, 6);
        }
        return;
    }

    bool felica = false;
    if (nfc_module == NFC_PN532) {
        felica = pn532_poll_felica(id, id + 8, id + 16, false);
    } else if (nfc_module == NFC_PN5180) {
        felica = pn5180_poll_felica(id, id + 8, id + 16, false);
    }
    if (felica) {
        hid_cardio.current[0] = REPORT_ID_FELICA;
        memcpy(hid_cardio.current + 1, id, 8);
        return;
    }

    bool vicinity = false;
    if (nfc_module == NFC_PN5180) {
        vicinity = pn5180_poll_vicinity(id, &len);
    }

    if (vicinity) {
        hid_cardio.current[0] = REPORT_ID_EAMU;
        memcpy(hid_cardio.current + 1, id, 8);
        return;
    }

    memset(hid_cardio.current, 0, 9);
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
    keypad_update();
    report_hid_key();

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

        keypad_update();
        detect_card();
    
        report_usb_hid();
    
        sleep_ms(1);
    }
}

void init()
{
    tusb_init();
    stdio_init_all();
    light_init();
    keypad_init();

    config_init();
    mutex_init(&core1_io_lock);
    save_init(0xca340a1c, &core1_io_lock);


    if (pn532_init(I2C_PORT, I2C_SCL, I2C_SDA, I2C_FREQ)) {
        nfc_module = NFC_PN532;
        pn532_set_wait_loop(wait_loop);
    } else if (pn5180_init(spi0, 16, 18, 19, 27, 17, 26)) {
        nfc_module = NFC_PN5180;
        pn5180_set_wait_loop(wait_loop);
    }

    aime_init(cdc_aime_putc);

    cli_init("aic_pico>", "\n     << AIC Pico >>\n"
                            " https://github.com/whowechina\n\n");
    
    commands_init();
}

/* if certain key pressed when booting, enter update mode */
static void update_check()
{
    const uint8_t pins[] = { 10, 11 }; // keypad 00 and *
    bool all_pressed = true;
    for (int i = 0; i < sizeof(pins); i++) {
        uint8_t gpio = pins[i];
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
        sleep_ms(1);
        if (gpio_get(gpio)) {
            all_pressed = false;
            break;
        }
    }

    if (all_pressed) {
        sleep_ms(100);
        reset_usb_boot(0, 2);
        return;
    }
}

static void sys_init()
{
    sleep_ms(50);
    set_sys_clock_khz(150000, true);
    board_init();
}

int main(void)
{
    sys_init();
    update_check();
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
