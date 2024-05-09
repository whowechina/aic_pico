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

#include <nfc.h>
#include <mode.h>
#include <aime.h>
#include <bana.h>

#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"
#include "light.h"
#include "keypad.h"

#define DEBUG(...) if (aic_runtime.debug) printf(__VA_ARGS__)

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

    if ((memcmp(hid_cardio.current, hid_cardio.reported, 9) != 0) &&
        (now - hid_cardio.report_time > 1000000)) {

        tud_hid_n_report(0x00, hid_cardio.current[0], hid_cardio.current + 1, 8);
        memcpy(hid_cardio.reported, hid_cardio.current, 9);
        hid_cardio.report_time = now;
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

static void light_effect()
{
    if (aime_is_active()) {
        light_set_rainbow(false);
        light_set_color(aime_led_color());
    } else if (bana_is_active()) {
        light_set_rainbow(false);
        light_set_color(bana_led_color());
    } else {
        if (memcmp(hid_cardio.current, "\0\0\0\0\0\0\0\0\0", 9) != 0) {
            light_rainbow(40, 0, aic_cfg->light.max);
            light_rainbow(1, 2500, aic_cfg->light.min);
        }
        light_set_rainbow(true);
    }

    light_update();
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            light_effect();
            mutex_exit(&core1_io_lock);
        }
        cli_fps_count(1);
        sleep_us(500);
    }
}

static void update_cardio(nfc_card_t *card)
{
    switch (card->card_type) {
        case NFC_CARD_MIFARE:
            hid_cardio.current[0] = REPORT_ID_EAMU;
            hid_cardio.current[1] = 0xe0;
            hid_cardio.current[2] = 0x04;
            if (card->len == 4) {
                memcpy(hid_cardio.current + 3, card->uid, 4);
                memcpy(hid_cardio.current + 7, card->uid, 2);
            } else if (card->len == 7) {
                memcpy(hid_cardio.current + 3, card->uid + 1, 6);
            }
            break;
        case NFC_CARD_FELICA:
            hid_cardio.current[0] = REPORT_ID_FELICA;
            memcpy(hid_cardio.current + 1, card->uid, 8);
            break;
        case NFC_CARD_VICINITY:
            hid_cardio.current[0] = REPORT_ID_EAMU;
            memcpy(hid_cardio.current + 1, card->uid, 8);
            break;
        default:
            memset(hid_cardio.current, 0, 9);
            return;
    }

    printf(" -> CardIO ");
    for (int i = 1; i < 9; i++) {
        printf("%02X", hid_cardio.current[i]);
    }
}

static void cardio_run()
{
    if (aime_is_active() || bana_is_active()) {
        memset(hid_cardio.current, 0, 9);
        return;
    }

    static nfc_card_t old_card = { 0 };

    nfc_rf_field(true);
    nfc_card_t card = nfc_detect_card();
    nfc_rf_field(false);

    if (memcmp(&old_card, &card, sizeof(old_card)) == 0) {
        return;
    }

    old_card = card;

    display_card(&card);
    update_cardio(&card);
}

const int aime_intf = 1;
static struct {
    uint8_t buf[64];
    int pos;
} aime;

static void cdc_aime_putc(uint8_t byte)
{
    tud_cdc_n_write(aime_intf, &byte, 1);
    tud_cdc_n_write_flush(aime_intf);
}

static void aime_poll_data()
{
    if (tud_cdc_n_available(aime_intf)) {
        int count = tud_cdc_n_read(aime_intf, aime.buf + aime.pos,
                                   sizeof(aime.buf) - aime.pos);
        if (count > 0) {
            uint32_t now = time_us_32();
            DEBUG("\n\033[32m%6ld>>", now / 1000);
            for (int i = 0; i < count; i++) {
                DEBUG(" %02X", aime.buf[aime.pos + i]);
            }
            DEBUG("\033[0m");
            aime.pos += count;
        }
    }
}

static void aime_detect_mode()
{
    if (aic_cfg->mode == MODE_AUTO) {
        static bool was_active = true; // so first time mode will be cleared
        bool is_active = aime_is_active() || bana_is_active();
        if (was_active && !is_active) {
            aic_runtime.mode = MODE_NONE;
        }
        was_active = is_active;
    } else {
        aic_runtime.mode = aic_cfg->mode;
    }

    if (aic_runtime.mode == MODE_NONE) {
        cdc_line_coding_t coding;
        tud_cdc_n_get_line_coding(aime_intf, &coding);
        aic_runtime.mode = mode_detect(aime.buf, aime.pos, coding.bit_rate);
        if ((aime.pos > 10) && (aic_runtime.mode == MODE_NONE)) {
            aime.pos = 0; // drop the buffer
        }
    }

}

static void aime_run()
{
    aime_poll_data();
    aime_detect_mode();

    if (aime.pos > 0) {
        uint8_t buf[64];
        memcpy(buf, aime.buf, aime.pos);
        int count = aime.pos;
        switch (aic_runtime.mode) {
            case MODE_AIME0:
            case MODE_AIME1:
                aime_sub_mode(aic_runtime.mode == MODE_AIME0 ? 0 : 1);
                for (int i = 0; i < count; i++) {
                    aime_feed(buf[i]);
                }
                aime.pos = 0;
                break;
            case MODE_BANA:
                for (int i = 0; i < count; i++) {
                    bana_feed(buf[i]);
                }
                aime.pos = 0;
                break;
            default:
                break;
        }
    }
}

void wait_loop()
{
    keypad_update();
    report_hid_key();

    tud_task();
    cli_run();
    aime_poll_data();

    cli_fps_count(0);
}

static void core0_loop()
{
    while(1) {
        tud_task();

        cli_run();
        aime_run();
        cardio_run();

        keypad_update();
        report_usb_hid();
    
        save_loop();
        cli_fps_count(0);
    
        sleep_ms(1);
    }
}

void init()
{
    tusb_init();
    stdio_init_all();

    config_init();
    mutex_init(&core1_io_lock);
    save_init(0xca340a1c, &core1_io_lock);

    light_init();
    light_rainbow(1, 0, aic_cfg->light.min);

    keypad_init();

    nfc_init_i2c(I2C_PORT, I2C_SCL, I2C_SDA, I2C_FREQ);
    nfc_init_spi(SPI_PORT, SPI_MISO, SPI_SCK, SPI_MOSI, SPI_RST, SPI_NSS, SPI_BUSY);
    nfc_init();
    nfc_set_wait_loop(wait_loop);

    aime_init(cdc_aime_putc);
    aime_virtual_aic(aic_cfg->virtual_aic);

    bana_init(cdc_aime_putc);

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
    if ((report_id == REPORT_ID_LIGHTS) &&
        (report_type == HID_REPORT_TYPE_OUTPUT)) {
        if (bufsize >= 3) {
            light_hid_light(buffer[0], buffer[1], buffer[2]);
        }
    }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    DEBUG("\nCDC Line State: %d %d", dtr, rts);
    aime_fast_expire();
    bana_fast_expire();
}
