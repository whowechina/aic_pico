/*
 * Cardio processing and auto PIN-entry
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "tusb.h"
#include "usb_descriptors.h"

#include "board_defs.h"

#include "nfc.h"
#include "gui.h"
#include "light.h"

#define LAST_CARD_TIMEOUT_US 60000000

static struct {
    uint8_t current[9];
    uint8_t reported[9];
    uint64_t report_time;
} hid_cardio;

static struct {
    uint8_t uid_len;
    uint8_t uid[8];
    uint64_t time;
} last_card;

static struct {
    bool active;
    uint8_t seq_len; // not pin length
    uint8_t index;
    struct {
        uint8_t key; // 0xff for empty
        uint8_t duration; // in cycles, most cases 1 cycle = 1ms
    } seq[32];
    uint64_t pin_time;
} autopin_ctx;

void cardio_report_cardio()
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

static void start_autopin(const char *pin, uint8_t delay)
{
    const char *keymap = KEYPAD_NKRO_MAP;
    autopin_ctx.active = true;
    autopin_ctx.index = 0;
    autopin_ctx.pin_time = time_us_64() + delay * 1000000;

    int len = 0;
    for (int i = 0; i < strlen(pin); i++) {
        int key = pin[i] - '0';
        int code = (key > 0) ? keymap[key - 1] : keymap[9];

        if ((i > 0) && (pin[i] == pin[i - 1])) {
            autopin_ctx.seq[len].key = 0xff; // release previous key
            autopin_ctx.seq[len].duration = 30;
            len++;
        }
        autopin_ctx.seq[len].key = code;
        autopin_ctx.seq[len].duration = 50;
        len++;
    }

    autopin_ctx.seq_len = len;
}

static void check_autopin(bool new_card)
{
    if (!aic_cfg->autopin.enabled) {
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (aic_cfg->autopin.entries[i].uidlen != last_card.uid_len) {
            continue;
        }

        if (memcmp(aic_cfg->autopin.entries[i].uid, last_card.uid, last_card.uid_len) != 0) {
            continue;
        }

        if (aic_cfg->autopin.entries[i].swipe && !new_card) {
            printf("\nPIN-entry launched.");
            start_autopin(aic_cfg->autopin.entries[i].pin, 0);
        } else {
            if (aic_cfg->autopin.entries[i].delay < 100) {
                printf("\nDelayed PIN-entry scheduled.");
                start_autopin(aic_cfg->autopin.entries[i].pin, aic_cfg->autopin.entries[i].delay);
            }
        }   
        break;
    }
}

bool cardio_autopin_rolling()
{
    if (!aic_cfg->autopin.enabled || !autopin_ctx.active) {
        return false;
    }

    if (time_us_64() < autopin_ctx.pin_time) {
        return false;
    }

    return true;
}

int cardio_get_pin_key()
{
    if (!cardio_autopin_rolling()) {
        return -1;
    }

    if (autopin_ctx.index >= autopin_ctx.seq_len) {
        autopin_ctx.active = false;
        return -1;
    }

    int key = autopin_ctx.seq[autopin_ctx.index].key;
    if (autopin_ctx.seq[autopin_ctx.index].duration > 0) {
        autopin_ctx.seq[autopin_ctx.index].duration--;
    } else {
        autopin_ctx.index++;
    }

    return key;
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

    gui_report_card_id(hid_cardio.current + 1, 8, true);
    printf(" -> CardIO ");
    for (int i = 1; i < 9; i++) {
        printf("%02X", hid_cardio.current[i]);
    }

    bool new_card = false;
    if ((card->len == last_card.uid_len) &&
        (memcmp(card->uid, last_card.uid, card->len) == 0)) {
        if (time_us_64() - last_card.time > LAST_CARD_TIMEOUT_US) {
            new_card = true;
        }
    } else {
        last_card.uid_len = card->len;
        memcpy(last_card.uid, card->uid, 8);
        new_card = true;
    }

    last_card.time = time_us_64();
  
    check_autopin(new_card);
}

void cardio_run(bool hid_light)
{
    static nfc_card_t old_card = { 0 };

    nfc_rf_field(true);
    nfc_card_t card = nfc_detect_card();
    if (card.card_type != NFC_CARD_NONE) {
        nfc_identify_last_card();
        gui_report_card_id(card.uid, card.len, false);
    }
    nfc_rf_field(false);

    if (memcmp(&old_card, &card, sizeof(old_card)) == 0) {
        return;
    }

    old_card = card;

    if (!hid_light) {
        if (card.card_type != NFC_CARD_NONE) {
            light_rainbow(30, 0, aic_cfg->light.level_active);
        } else {
            light_rainbow(1, 3000, aic_cfg->light.level_idle);
        }
    }

    display_card(&card);
    update_cardio(&card);
}

void cardio_clear()
{
    memset(hid_cardio.current, 0, 9);
    memset(&autopin_ctx, 0, sizeof(autopin_ctx));
}

bool cardio_get_last(uint8_t uid[8], uint8_t *len)
{
    if (time_us_64() - last_card.time > LAST_CARD_TIMEOUT_US) {
        return false;
    }

    if (last_card.uid_len == 0) {
        return false;
    }

    *len = last_card.uid_len;
    memcpy(uid, last_card.uid, last_card.uid_len);

    return true;
}
