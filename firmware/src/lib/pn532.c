/*
 * PN532 NFC Reader
 * WHowe <github.com/whowechina>
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "pn532.h"

#define DEBUG(...) { if (0) printf(__VA_ARGS__); }

#define IO_TIMEOUT_US 1000
#define PN532_I2C_ADDRESS 0x24

#define PN532_PREAMBLE 0
#define PN532_STARTCODE1 0
#define PN532_STARTCODE2 0xff
#define PN532_POSTAMBLE 0

#define PN532_HOSTTOPN532 0xd4
#define PN532_PN532TOHOST 0xd5

static i2c_inst_t *i2c_port = i2c0;
static uint32_t version = 0;

static uint32_t read_firmware_ver()
{
    uint8_t buf[4];
    if ((pn532_write_command(0x02, NULL, 0) >= 0) &&
        (pn532_read_response(0x02, buf, sizeof(buf)) == 4)) {
        return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    }
    return 0;
}

bool pn532_init(i2c_inst_t *i2c)
{
    i2c_port = i2c;
    version = read_firmware_ver();

    return (version > 0) && (version < 0x7fffffff);
}

static pn532_wait_loop_t wait_loop = NULL;

void pn532_set_wait_loop(pn532_wait_loop_t loop)
{
    wait_loop = loop;
}

static int pn532_write(const uint8_t *data, uint8_t len)
{
    return i2c_write_blocking_until(i2c_port, PN532_I2C_ADDRESS, data, len, false,
                             time_us_64() + IO_TIMEOUT_US * len);
}

static int pn532_read(uint8_t *data, uint8_t len)
{
    return i2c_read_blocking_until(i2c_port, PN532_I2C_ADDRESS, data, len, false,
                            time_us_64() + IO_TIMEOUT_US * len);
}

static bool pn532_wait_ready()
{
    uint8_t status = 0;

    for (int retry = 0; retry < 30; retry++) {
        if (pn532_read(&status, 1) == 1 && status == 0x01) {
            return true;
        }
        if (wait_loop) {
            wait_loop();
        }
        sleep_us(1000);
    }

    return false;
}

static int read_frame(uint8_t *frame, uint8_t len)
{
    uint8_t buf[len + 1];
    int ret = pn532_read(buf, len + 1);

    if (ret == len + 1) {
        memcpy(frame, buf + 1, len);
        return len;
    }
    return ret;
}

static int write_frame(const uint8_t *frame, uint8_t len)
{
    return pn532_write(frame, len);
}

static bool read_ack()
{
    uint8_t resp[6];

    if (!pn532_wait_ready()) {
        return false;
    }

    read_frame(resp, 6);

    const uint8_t expect_ack[] = {0, 0, 0xff, 0, 0xff, 0};
    if (memcmp(resp, expect_ack, 6) != 0) {
        return false;
    }
   
    return true;
}

int pn532_write_data(const uint8_t *data, uint8_t len)
{
    uint8_t frame[7 + len];
    frame[0] = PN532_PREAMBLE;
    frame[1] = PN532_STARTCODE1;
    frame[2] = PN532_STARTCODE2;
    uint8_t checksum = 0xff;

    frame[3] = len;
    frame[4] = (~len + 1);

    for (int i = 0; i < len; i++) {
        frame[5 + i] = data[i];
        checksum += data[i];
    }

    frame[5 + len] = ~checksum;
    frame[6 + len] = PN532_POSTAMBLE;

    write_frame(frame, 7 + len);

    return read_ack();
}

int pn532_read_data(uint8_t *data, uint8_t len)
{
    uint8_t resp[len + 7];

    read_frame(resp, len + 7);

    if (resp[0] != PN532_PREAMBLE ||
        resp[1] != PN532_STARTCODE1 ||
        resp[2] != PN532_STARTCODE2) {
        return -1;
    }

    uint8_t length = resp[3];
    uint8_t length_check = length + resp[4];

    if (length > len ||
        length_check != 0 ||
        resp[length + 6] != PN532_POSTAMBLE) {
        return -1;
    }

    uint8_t checksum = 0;
    for (int i = 0; i <= length; i++) {
        data[i] = resp[5 + i];
        checksum += resp[5 + i];
    }

    if (checksum != 0) {
        return -1;
    }
    
    return length;
}

int pn532_write_command(uint8_t cmd, const uint8_t *param, uint8_t len)
{
    uint8_t data[len + 2];
    data[0] = PN532_HOSTTOPN532;
    data[1] = cmd;

    memcpy(data + 2, param, len);

    return pn532_write_data(data, len + 2);
}

static void write_nack()
{
    const uint8_t nack[] = {0, 0, 0xff, 0xff, 0, 0};
    pn532_write(nack, 6);
}

int pn532_peak_response_len()
{
    uint8_t buf[6];
    if (!pn532_wait_ready()) {
        return -1;
    }
    pn532_read(buf, 6);
    if (buf[0] != 0x01 ||
        buf[1] != PN532_PREAMBLE ||
        buf[2] != PN532_STARTCODE1 ||
        buf[3] != PN532_STARTCODE2) {
        return -1;
    }

    write_nack();
    return buf[4];
}

int pn532_read_response(uint8_t cmd, uint8_t *resp, uint8_t len)
{
    int real_len = pn532_peak_response_len();
    if (real_len < 0) {
        return -1;
    }

    if (!pn532_wait_ready()) {
        return -1;
    }

    if (real_len < 2) {
        return -1;
    }

    uint8_t data[real_len];
    int ret = pn532_read_data(data, real_len);
    if (ret != real_len ||
        data[0] != PN532_PN532TOHOST ||
        data[1] != cmd + 1) {
        return -1;
    }

    int data_len = real_len - 2;
    if (data_len > len) {
        return -1;
    }

    if (data_len <= 0) {
        return -1;
    }
    
    memcpy(resp, data + 2, data_len);

    return data_len;
}

const char *pn532_firmware_ver()
{
    static char ver_str[16];
    sprintf(ver_str, "%08lX", version);
    return ver_str;
}

bool pn532_config_rf()
{
    uint8_t param[] = {0x05, 0xff, 0x01, 0x50};
    pn532_write_command(0x32, param, sizeof(param));

    return pn532_read_response(0x32, param, sizeof(param)) == 0;
}

bool pn532_config_sam()
{
    uint8_t param[] = {0x01, 0x14, 0x01};
    pn532_write_command(0x14, param, sizeof(param));

    uint8_t resp;
    return pn532_read_response(0x14, &resp, 1) == 0;
}

static bool pn532_set_rf_field(bool auto_rf, bool on_off)
{
    uint8_t param[] = { 1, (auto_rf ? 2 : 0) | (on_off ? 1 : 0) };
    pn532_write_command(0x32, param, 2);

    uint8_t resp;
    return pn532_read_response(0x32, &resp, 1) >= 0;
}

void pn532_rf_field(bool on)
{
    pn532_set_rf_field(true, on);
    if (on) {
        pn532_config_sam();
    }
}

static uint8_t readbuf[255];

bool pn532_poll_mifare(uint8_t uid[7], int *len)
{
    uint8_t param[] = {0x01, 0x00};
    int ret = pn532_write_command(0x4a, param, sizeof(param));
    if (ret < 0) {
        return false;
    }

    int result = pn532_read_response(0x4a, readbuf, sizeof(readbuf));
    if (result < 1 || readbuf[0] != 1) {
        return false;
    }

    int idlen = readbuf[5];
    if ((idlen > 8) || (result != idlen + 6)) {
        return false;
    }

    memcpy(uid, readbuf + 6, idlen);
    *len = idlen;

    return true;
}

static struct __attribute__((packed)) {
    uint8_t idm[8];
    uint8_t pmm[8];
    uint8_t syscode[2];
    uint8_t inlist_tag;
} felica_poll_cache;

bool pn532_poll_felica(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache)
{
    if (from_cache) {
        memcpy(uid, felica_poll_cache.idm, 8);
        memcpy(pmm, felica_poll_cache.pmm, 8);
        memcpy(syscode, felica_poll_cache.syscode, 2);
        return true;
    }

    uint8_t param[] = { 1, 1, 0, 0xff, 0xff, 1, 0};
    int ret = pn532_write_command(0x4a, param, sizeof(param));
    if (ret < 0) {
        return false;
    }

    int result = pn532_read_response(0x4a, readbuf, sizeof(readbuf));

    if ((result != 22) || (readbuf[0] != 1) || (readbuf[2] != 20)) {
        return false;
    }

    memcpy(&felica_poll_cache, readbuf + 4, 18);
    felica_poll_cache.inlist_tag = readbuf[1];

    memcpy(uid, readbuf + 4, 8);
    memcpy(pmm, readbuf + 12, 8);
    memcpy(syscode, readbuf + 20, 2);

    return true;
}

bool pn532_mifare_auth(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t key[6])
{
    uint8_t param[] = {
        1, key_id ? 0x61 : 0x60, block_id,
        key[0], key[1], key[2], key[3], key[4], key[5],
        uid[0], uid[1], uid[2], uid[3]
    };

    int ret = pn532_write_command(0x40, param, sizeof(param));
    if (ret < 0) {
        DEBUG("\nPN532 failed mifare auth command");
        return false;
    }
    int result = pn532_read_response(0x40, readbuf, sizeof(readbuf));
    if (readbuf[0] != 0) {
        DEBUG("\nPN532 Mifare AUTH failed %d %02x key[%2x:%d]: ", result, readbuf[0], param[1], param[2]);
        for (int i = 0; i < 6; i++) {
            DEBUG("%02x", key[i]);
        }
        return false;
    }

    return true;
}

bool pn532_mifare_read(uint8_t block_id, uint8_t block_data[16])
{
    uint8_t param[] = { 1, 0x30, block_id };

    int ret = pn532_write_command(0x40, param, sizeof(param));
    if (ret < 0) {
        DEBUG("\nPN532 failed mifare read command");
        return false;
    }

    int result = pn532_read_response(0x40, readbuf, sizeof(readbuf));

    if (readbuf[0] != 0 || result != 17) {
        DEBUG("\nPN532 Mifare READ failed %d %02x", result, readbuf[0]);
        return false;
    }

    memmove(block_data, readbuf + 1, 16);

    return true;
}

int pn532_felica_command(uint8_t cmd, const uint8_t *param, uint8_t param_len, uint8_t *outbuf)
{
    int cmd_len = param_len + 11;
    uint8_t cmd_buf[cmd_len + 1];

    cmd_buf[0] = felica_poll_cache.inlist_tag;
    cmd_buf[1] = cmd_len;
    cmd_buf[2] = cmd;
    memcpy(cmd_buf + 3, felica_poll_cache.idm, 8);
    memcpy(cmd_buf + 11, param, param_len);

    int ret = pn532_write_command(0x40, cmd_buf, sizeof(cmd_buf));
    if (ret < 0) {
        DEBUG("\nFailed send felica command");
        return -1;
    }

    int result = pn532_read_response(0x40, readbuf, sizeof(readbuf));

    int outlen = readbuf[1] - 1;
    if ((readbuf[0] & 0x3f) != 0 || result - 2 != outlen) {
        return -1;
    }

    memmove(outbuf, readbuf + 2, outlen);

    return outlen;
}


bool pn532_felica_read(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16])
{
    uint8_t param[] = { 1, svc_code & 0xff, svc_code >> 8,
                        1, block_id >> 8, block_id & 0xff };

    int result = pn532_felica_command(0x06, param, sizeof(param), readbuf);

    if (result != 12 + 16 || readbuf[9] != 0 || readbuf[10] != 0) {
        DEBUG("\nPN532 Felica read failed [%04x:%04x]", svc_code, block_id);
        memset(block_data, 0, 16);
        return true; // we fake the result when it fails
    }

    const uint8_t *result_data = readbuf + 12; 
    memcpy(block_data, result_data, 16);

    return true;
}

bool pn532_felica_write(uint16_t svc_code, uint16_t block_id, const uint8_t block_data[16])
{
    uint8_t param[22] = { 1, svc_code & 0xff, svc_code >> 8,
                        1, block_id >> 8, block_id & 0xff };
    memcpy(param + 6, block_data, 16);
    int result = pn532_felica_command(0x08, param, sizeof(param), readbuf);

    if (result < 0) {
        DEBUG("\nPN532 Felica WRITE failed %d", result);
        return false;
    }

    DEBUG("\nPN532 Felica WRITE success ");
    for (int i = 0; i < result; i++) {
        printf(" %02x", readbuf[i]);
    }
    return false;
}

void pn532_select()
{
    uint8_t ignore_buf[7];
    int ignore_len;
    pn532_poll_mifare(ignore_buf, &ignore_len);
}
