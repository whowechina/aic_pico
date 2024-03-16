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
#define CMD_LOAD_RF_CONFIG 0x11
#define CMD_RF_ON 0x16
#define CMD_RF_OFF 0x17

static spi_inst_t *spi_port;
static uint8_t gpio_rst;
static uint8_t gpio_nss;
static uint8_t gpio_busy;

bool pn5180_init(spi_inst_t *port, uint8_t miso, uint8_t sck, uint8_t mosi,
                 uint8_t rst, uint8_t nss, uint8_t busy)
{
    spi_init(port, 2000 * 1000);
    spi_set_format(port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);

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

void pn5180_rf_on()
{
    uint8_t buf[] = { CMD_RF_ON, 0 };
    read_write(buf, sizeof(buf), NULL, 0);
}

void pn5180_rf_off()
{
    uint8_t buf[] = { CMD_RF_OFF, 0 };
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
    pn5180_rf_on();

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

    pn5180_rf_off();

    return result;
}

bool pn5180_poll_felica(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache)
{
    pn5180_reset();
    pn5180_load_rf_config(0x08, 0x88);
    pn5180_rf_on();

    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xffffffbf);
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03);

    uint8_t cmd[] = {0x06, 0x00, 0xff, 0xff, 0x01, 0x00};

	pn5180_send_data(cmd, 6, 0x00);

    sleep_ms(1);

    uint8_t out[20] = {0};
    pn5180_read_data(out, 20);

    bool result = false;
    if (out[1] == 0x01) {
        result = true;
        memmove(uid, out + 2, 8);
        memmove(pmm, out + 10, 8);
        memmove(syscode, out + 18, 2);
        result = true;
    }

    pn5180_rf_off();

    return result;
}

bool pn5180_poll_vicinity(uint8_t uid[8])
{
    pn5180_reset();
    pn5180_load_rf_config(0x0d, 0x8d);
    pn5180_rf_on();

    pn5180_clear_irq(0x0fffff);
    pn5180_and_reg(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8);
    pn5180_or_reg(PN5180_REG_SYSTEM_CONFIG, 0x03);

    uint8_t cmd[] = {0x26, 0x01, 0x00};
    pn5180_send_data(cmd, 3, 0);

    sleep_ms(1);

    if ((pn5180_get_irq() & 0x4000) == 0) {
        pn5180_rf_off();
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

    pn5180_rf_off();
    
    return result;
}

void pn5180_print_rf_cfg()
{
    printf("RF_CONTROL_TX_CLK: %08lx\n", pn5180_read_reg(0x21));
    printf("TX_DATA_MOD:       %08lx\n", pn5180_read_reg(0x16));
    printf("TX_UNDERSHOOT_CFG: %08lx\n", pn5180_read_reg(0x14));
    printf("TX_OVERSHOOT_CFG:  %08lx\n", pn5180_read_reg(0x15));
    printf("RF_CONTROL_TX:     %08lx\n", pn5180_read_reg(0x20));
    printf("ANT_CONTROL:       %08lx\n", pn5180_read_reg(0x29));
}