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

#include "pn5180.h"

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

    uint8_t buf[2];
    pn5180_read_eeprom(0x12, buf, sizeof(buf));
    return (buf[0] <= 15) && (buf[1] >= 2) && (buf[1] <= 15);
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

static bool read_write(const uint8_t *data, uint8_t len, uint8_t *buf, uint8_t buf_len)
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

static void anti_collision(uint8_t code, uint8_t uid[5], uint8_t *sak)
{
    rf_crc_off();
    uint8_t cmd[7] = { code, 0x20 };
    pn5180_send_data(cmd, 2, 0);
    pn5180_read_data(cmd + 2, 5); // uid
    memmove(uid, cmd + 2, 5);

    rf_crc_on();
    cmd[0] = code;
    cmd[1] = 0x70;
    pn5180_send_data(cmd, 7, 0);
    pn5180_read_data(sak, 1); // sak
}

bool pn5180_poll_mifare(uint8_t uid[7], int *len)
{
    pn5180_reset();
    pn5180_load_rf_config(0x00, 0x80);
    pn5180_rf_field(true);

    rf_crc_off();

    pn5180_and_reg(PN5180_REG_IRQ_CLEAR, 0x000fffff);
    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8);
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03);

    uint8_t cmd[1] = {0x26};
    pn5180_send_data(cmd, 1, 7);
    uint8_t buf[5] = {0};
    pn5180_read_data(buf, 2);

    uint8_t sak;

    anti_collision(0x93, buf, &sak);

    bool result = false;
    if ((sak & 0x04) == 0) {
        memmove(uid, buf, 4);
        *len = 4;
        result = true;
    } else if (buf[0] == 0x88) {
        memmove(uid, buf + 1, 3);
        anti_collision(0x95, buf, &sak);
        if (sak != 0xff) {
            memmove(uid + 3, buf, 4);
            *len = 7;
            result = true;
        }
    }

    return result;
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
    uint8_t cmd[] = {
        CMD_MIFARE_AUTHENTICATE,
        key[0], key[1], key[2], key[3], key[4], key[5],
        key_id ? 0x61 : 0x60, block_id,
        uid[0], uid[1], uid[2], uid[3]
    };

    uint8_t response = 0;
    read_write(cmd, sizeof(cmd), &response, 1);

    if ((response == 1) || (response == 2)) {
        printf("\nMifare auth failed: %d, [%d:%d]", response, cmd[7], block_id);
        return false;
    }

    return true;
}

bool pn5180_mifare_read(uint8_t block_id, uint8_t block_data[16])
{
    uint8_t cmd[] = { CMD_MIFARE_READ, block_id };
    pn5180_send_data(cmd, sizeof(cmd), 0);

    sleep_ms_with_loop(5);

    uint16_t len = pn5180_get_rx() & 0x1ff;
    if (len != 16) {
        printf("\nMifare read error (block %d): %d", block_id, len);
        return false;
    }

    pn5180_read_data(block_data, 16);
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
        printf("\nPN532 Felica read failed [%04x:%04x]", svc_code, block_id);
        memset(block_data, 0, 16);
        return false;
    }

    memcpy(block_data, out.data, 16);
    return true;
}

