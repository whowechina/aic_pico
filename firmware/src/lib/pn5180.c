/*
 * PN5180 NFC Reader
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "nfc.h"
#include "pn5180.h"

#define DEBUG(...) { if (nfc_runtime.debug) printf(__VA_ARGS__); }

#define IO_TIMEOUT_US 1000
#define PN5180_I2C_ADDRESS 0x24

#define CMD_WRITE_REG 0x00
#define CMD_WRITE_REG_OR 0x01
#define CMD_WRITE_REG_AND 0x02
#define CMD_READ_REG 0x04
#define CMD_WRITE_EEPROM 0x06
#define CMD_READ_EEPROM 0x07
#define CMD_SEND_DATA 0x09
#define CMD_READ_DATA 0x0a
#define CMD_MIFARE_AUTHENTICATE 0x0c
#define CMD_LOAD_RF_CONFIG 0x11
#define CMD_RF_ON 0x16
#define CMD_RF_OFF 0x17
#define CMD_MIFARE_READ 0x30

static spi_inst_t *spi_port;
static uint8_t gpio_rst;
static uint8_t gpio_nss;
static uint8_t gpio_busy;
static uint8_t version[2];

bool pn5180_init(spi_inst_t *port, uint8_t rst, uint8_t nss, uint8_t busy)
{
    gpio_init(nss);
    gpio_set_dir(nss, GPIO_OUT);
    gpio_pull_up(nss);
    gpio_put(nss, 1);

    gpio_init(rst);
    gpio_set_dir(rst, GPIO_OUT);
    gpio_pull_up(rst);
    gpio_put(rst, 1);

    spi_port = port;
    gpio_rst = rst;
    gpio_nss = nss;
    gpio_busy = busy;

    pn5180_read_eeprom(0x12, version, sizeof(version));
    return (version[0] <= 15) && (version[1] >= 2) && (version[1] <= 15);
}

const char *pn5180_firmware_ver()
{
    static char ver_str[8];
    sprintf(ver_str, "%d.%d", version[1], version[0]);
    return ver_str;
}

static pn5180_wait_loop_t wait_loop = NULL;

static inline void wait_not_busy()
{
    int count = 0;
    while (gpio_get(gpio_busy)) {
        sleep_us(10);
        count += 10;
        if ((count > 1000) && wait_loop) {
            wait_loop();
            count = 0;
        }
    }
}

static void sleep_ms_with_loop(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        sleep_ms(1);
        if (wait_loop) {
            wait_loop();
        }
    }
}

static inline void begin_transmission()
{
    wait_not_busy();
    gpio_put(gpio_nss, 0);
    sleep_ms_with_loop(2);
}

static inline void end_transmission()
{
    gpio_put(gpio_nss, 1);
    sleep_ms_with_loop(3);
}

void pn5180_set_wait_loop(pn5180_wait_loop_t loop)
{
    wait_loop = loop;
}

static bool read_write(const void *data, uint8_t len, uint8_t *buf, uint8_t buf_len)
{
    begin_transmission();
    spi_write_blocking(spi_port, data, len);
    end_transmission();

    if (!buf || (buf_len == 0)) {
        return true;
    }

    begin_transmission();
    spi_read_blocking(spi_port, 0, buf, buf_len);
    end_transmission();

    return true;
}

static bool write_reg(uint8_t cmd, uint8_t reg, uint32_t v32)
{
    uint8_t buf[] = { cmd, reg, v32 & 0xff, (v32 >> 8) & 0xff,
                                (v32 >> 16) & 0xff, (v32 >> 24) & 0xff };
    return read_write(buf, sizeof(buf), NULL, 0);
}

void pn5180_write_reg(uint8_t reg, uint32_t v32)
{
    write_reg(CMD_WRITE_REG, reg, v32);
}

void pn5180_or_reg(uint8_t reg, uint32_t mask)
{
    write_reg(CMD_WRITE_REG_OR, reg, mask);
}

void pn5180_and_reg(uint8_t reg, uint32_t mask)
{
    write_reg(CMD_WRITE_REG_AND, reg, mask);
}

uint32_t pn5180_read_reg(uint8_t reg)
{
    uint8_t buf[] = { CMD_READ_REG, reg };
    uint8_t out[4];
    read_write(buf, sizeof(buf), out, sizeof(out));
    return out[0] | (out[1] << 8) | (out[2] << 16) | (out[3] << 24);
}

void pn5180_send_data(const uint8_t *data, uint8_t len, uint8_t last_bits)
{
    uint8_t buf[len + 2];
    buf[0] = CMD_SEND_DATA;
    buf[1] = last_bits;
    memmove(buf + 2, data, len);
    read_write(buf, sizeof(buf), NULL, 0);
}

void pn5180_read_data(uint8_t *data, uint8_t len)
{
    uint8_t buf[] = { CMD_READ_DATA, 0x00 };
    read_write(buf, sizeof(buf), data, len);
}

void pn5180_read_eeprom(uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t cmd[3] = { CMD_READ_EEPROM, addr, len };
    read_write(cmd, sizeof(cmd), buf, len);
}

void pn5180_load_rf_config(uint8_t tx_cfg, uint8_t rx_cfg)
{
    uint8_t buf[] = { CMD_LOAD_RF_CONFIG, tx_cfg, rx_cfg};
    read_write(buf, sizeof(buf), NULL, 0);
}

void pn5180_rf_field(bool on)
{
    uint8_t buf[] = { on ? CMD_RF_ON : CMD_RF_OFF, 0 };
    read_write(buf, sizeof(buf), NULL, 0);   
}

void pn5180_reset()
{
    gpio_put(gpio_rst, 0);
    sleep_us(20);
    gpio_put(gpio_rst, 1);
    sleep_ms(1);
    while ((pn5180_get_irq() & (1 << 2)) == 0) {
        if (wait_loop) {
            wait_loop();
        }
        sleep_ms(1);
    }

    pn5180_clear_irq(0xffffffff); // clear all flags
}

uint32_t pn5180_get_irq()
{
    return pn5180_read_reg(PN5180_REG_IRQ_STATUS);
}

void pn5180_clear_irq(uint32_t mask)
{
    pn5180_write_reg(PN5180_REG_IRQ_CLEAR, mask);
}

uint32_t pn5180_get_rx()
{
    return pn5180_read_reg(PN5180_REG_RX_STATUS);
}

static void rf_crc_off()
{
    pn5180_and_reg(PN5180_REG_CRC_TX_CONFIG, 0xfffffffe);
    pn5180_and_reg(PN5180_REG_CRC_RX_CONFIG, 0xfffffffe);
}

static void rf_crc_on()
{
    pn5180_or_reg(PN5180_REG_CRC_TX_CONFIG, 0x01);
    pn5180_or_reg(PN5180_REG_CRC_RX_CONFIG, 0x01);
}

static struct {
    uint8_t atqa[2];
    uint8_t buf[5];
    uint8_t sak;
    uint8_t uid[7];
    uint8_t len;
    bool ready;
} mi_poll;

static bool anti_collision(uint8_t code, uint8_t uid[5], uint8_t *sak)
{
    rf_crc_off();
    uint8_t cmd[7] = { code, 0x20 };
    pn5180_send_data(cmd, 2, 0);
    if ((pn5180_get_rx() & 0x1ff) != 5) {
        return false;
    }
    pn5180_read_data(cmd + 2, 5); // uid
    memmove(uid, cmd + 2, 5);

    rf_crc_on();
    cmd[0] = code;
    cmd[1] = 0x70;
    pn5180_send_data(cmd, 7, 0);
    if ((pn5180_get_rx() & 0x1ff) != 1) {
        return false;
    }
    pn5180_read_data(sak, 1); // sak
    return true;
}

static void poll_mifare_0()
{
    pn5180_reset();
    pn5180_load_rf_config(0x00, 0x80);
    if (nfc_runtime.pn5180_tx_tweak) {
        write_reg(CMD_WRITE_REG, 0x21, 0x783); // Enable DPLL
    }
    pn5180_rf_field(true);
    rf_crc_off();
}

static void poll_mifare_1()
{
    pn5180_and_reg(PN5180_REG_IRQ_CLEAR, 0x000fffff);
    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8);
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03);

    mi_poll.len = 0;
    mi_poll.ready = true;

    uint8_t cmd[1] = {0x26};
    pn5180_send_data(cmd, 1, 7);
    if ((pn5180_get_rx() & 0x1ff) != 2) {
        mi_poll.ready = false;
        return;
    }
    pn5180_read_data(mi_poll.atqa, 2);
}

static void poll_mifare_2()
{
    if (!mi_poll.ready) {
        return;
    }

    if (!anti_collision(0x93, mi_poll.buf, &mi_poll.sak)) {
        return;
    }

    mi_poll.len = 0;
    if ((mi_poll.sak & 0x04) == 0) {
        memmove(mi_poll.uid, mi_poll.buf, 4);
        mi_poll.len = 4;
    } else if (mi_poll.buf[0] == 0x88) {
        memmove(mi_poll.uid, mi_poll.buf + 1, 3);
        if (!anti_collision(0x95, mi_poll.buf, &mi_poll.sak)) {
            return;
        }
        if (mi_poll.sak != 0xff) {
            memmove(mi_poll.uid + 3, mi_poll.buf, 4);
            mi_poll.len = 7;
        }
    }
}

bool pn5180_poll_mifare(uint8_t uid[7], int *len)
{
    poll_mifare_0();
    poll_mifare_1();
    poll_mifare_2();

    memcpy(uid, mi_poll.uid, mi_poll.len);
    *len = mi_poll.len;
    return *len > 0;
}

static uint8_t idm_cache[8] = {0};

bool pn5180_poll_felica(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache)
{
    pn5180_reset();
    pn5180_load_rf_config(0x09, 0x89);
    pn5180_rf_field(true);

    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xffffffbf);
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03);

    uint8_t cmd[] = {0x06, 0x00, 0xff, 0xff, 0x01, 0x00};

	pn5180_send_data(cmd, sizeof(cmd), 0x00);
    sleep_ms(1);

    struct __attribute__((packed)) {
        uint8_t len;
        uint8_t cmd;
        uint8_t idm[8];
        uint8_t pmm[8];
        uint8_t syscode[2];
    } out = { 0 };
    pn5180_read_data((uint8_t *)&out, sizeof(out));

    if ((out.len != sizeof(out)) || (out.cmd != 0x01)) {
        return false;
    }

    memcpy(uid, out.idm, 8);
    memcpy(pmm, out.pmm, 8);
    memcpy(syscode, out.syscode, 2);

    /* double check the result */
	pn5180_send_data(cmd, sizeof(cmd), 0x00);
    sleep_ms(1);
    pn5180_read_data((uint8_t *)&out, sizeof(out));
    if ((out.len != sizeof(out)) || (out.cmd != 0x01) ||
        (memcmp(uid, out.idm, 8) != 0)) {
        return false;
    }

    memcpy(idm_cache, uid, 8);
    return true;
}

bool pn5180_poll_vicinity(uint8_t uid[8])
{
    pn5180_reset();
    pn5180_load_rf_config(0x0d, 0x8d);
    pn5180_rf_field(true);

    pn5180_clear_irq(0x0fffff);
    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8);
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03);

    uint8_t cmd[] = {0x26, 0x01, 0x00};
    pn5180_send_data(cmd, 3, 0);

    sleep_ms(1);

    if ((pn5180_get_irq() & 0x4000) == 0) {
        pn5180_rf_field(false);
        return false;
    }

    while ((pn5180_get_irq() & 0x01) == 0) {
        if (wait_loop) {
            wait_loop();
        }
        sleep_ms(1);
    }

    int len = pn5180_get_rx() & 0x1ff;

    bool result = false;
    if (len == 10) {
        uint8_t id[10];
        pn5180_read_data(id, len);
        for (int i = 0; i < 8; i++) {
            uid[i] = id[9 - i]; // 15693 stores id in reversed byte order
        }
        result = true;
    }

    return result;
}

bool pn5180_mifare_auth(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t key[6])
{
    static struct {
        uint32_t time;
        uint8_t uid[4];
        uint8_t key_id;
        uint8_t block_id;
        bool result;
    } cache = { 0 };

    uint32_t now = time_us_32();
    if ((now < cache.time + 1000000) &&
        (memcmp(cache.uid, uid, 4) == 0) &&
        (cache.key_id == key_id) &&
        (cache .block_id == block_id)) {
        cache.time = now;
        return cache.result;
    }

    cache.time = now;
    memcpy(cache.uid, uid, 4);
    cache.key_id = key_id;
    cache.block_id = block_id;
    cache.result = false;

    struct __attribute__((packed)) {
        uint8_t cmd;
        uint8_t key[6];
        uint8_t key_id;
        uint8_t block_id;
        uint8_t uid[4];
    } cmd = {
        .cmd = CMD_MIFARE_AUTHENTICATE,
        .key = { key[0], key[1], key[2], key[3], key[4], key[5] },
        .key_id = key_id ? 0x61 : 0x60,
        .block_id = block_id,
        .uid = { uid[0], uid[1], uid[2], uid[3] }
    };

    uint8_t response = 0;
    read_write(&cmd, sizeof(cmd), &response, 1);

    if ((response == 1) || (response == 2)) {
        DEBUG("\nPN5180 Mifare auth failed: %d, [%02x:%d]", response, cmd.key_id, cmd.block_id);
        return false;
    }

    uint8_t ignored[16];
    if (!pn5180_mifare_read((block_id & 0xfc) + 1, ignored)) {
        DEBUG("\nPN5180 Mifare auth check bad");
        return false;
    }

    cache.result = true;

    return true;
}

bool pn5180_mifare_read(uint8_t block_id, uint8_t block_data[16])
{
    static struct {
        uint8_t block_id;
        uint32_t time;
        uint8_t data[16];
    } cache[3] = { 0 };

    uint32_t now = time_us_32();

    if (block_id < 3) {
        if ((cache[block_id].block_id == block_id) &&
            (now < cache[block_id].time + 1000000)) {
            memcpy(block_data, cache[block_id].data, 16);
            cache[block_id].time = now;
            return true;
        }
    }

    uint8_t cmd[] = { CMD_MIFARE_READ, block_id };
    pn5180_send_data(cmd, sizeof(cmd), 0);

    sleep_ms_with_loop(5);

    uint16_t len = pn5180_get_rx() & 0x1ff;
    if (len != 16) {
        DEBUG("\nPN5180 Mifare read error (block %d): %d", block_id, len);
        return false;
    }

    pn5180_read_data(block_data, 16);

    if (block_id < 3) {
        memcpy(cache[block_id].data, block_data, 16);
        cache[block_id].block_id = block_id;
        cache[block_id].time = time_us_32();
    }

    return true;
}

bool pn5180_felica_read(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16])
{
    uint8_t cmd[] = {0x10, 0x06, 
                    idm_cache[0], idm_cache[1],
                    idm_cache[2], idm_cache[3],
                    idm_cache[4], idm_cache[5],
                    idm_cache[6], idm_cache[7],
                    0x01, svc_code & 0xff, svc_code >> 8,
                    0x01, block_id >> 8, block_id & 0xff};

	pn5180_send_data(cmd, sizeof(cmd), 0x00);
    sleep_ms(1);

    struct __attribute__((packed)) {
        uint8_t len;
        uint8_t cmd;
        uint8_t idm[8];
        uint16_t status;
        uint8_t block_num;
        uint8_t data[16];
    } out = { 0 };
    pn5180_read_data((uint8_t *)&out, sizeof(out));

    if ((out.len != sizeof(out)) || (out.cmd != 0x07) || (out.status != 0x00)) {
        DEBUG("\nPN5180 Felica read failed [%04x:%04x]", svc_code, block_id);
        memset(block_data, 0, 16);
        return false;
    }

    memcpy(block_data, out.data, 16);
    return true;
}

void pn5180_select(int phase)
{
    if (phase == 0) {
        poll_mifare_0();
    } else {
        poll_mifare_1();
    }
}

void pn5180_deselect()
{
    poll_mifare_2();
}

bool pn5180_15693_read(const uint8_t uid[8], uint8_t block_id, uint8_t block_data[4])
{
    uint8_t cmd[] = { 0x22, 0x20, uid[7], uid[6], uid[5], uid[4],
                      uid[3], uid[2], uid[1], uid[0], block_id } ;
    pn5180_send_data(cmd, sizeof(cmd), 0);

    sleep_ms_with_loop(5);

    uint32_t status = pn5180_get_irq();
    if (0 == (status & 0x4000)) {
        return false;
    }

    while (0 == (status & 0x01)) {
        sleep_ms_with_loop(5);
        status = pn5180_get_irq();
    }
  
    uint16_t len = pn5180_get_rx() & 0x1ff;

    uint8_t buf[5] = { 0 };

    if (len > sizeof(buf)) {
        return false;
    }

    pn5180_read_data(buf, len);
    if ((buf[0] & 0x01) != 0) {
        return false;
    }

    memcpy(block_data, buf + 1, 4);

    return true;
}
