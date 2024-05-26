/*
 * PN532 NFC Reader
 * WHowe <github.com/whowechina>
 * 
 */

#ifndef PN532_H
#define PN532_H

#include <stdint.h>
#include "hardware/i2c.h"

typedef void (*pn532_wait_loop_t)();

void pn532_set_wait_loop(pn532_wait_loop_t loop);

bool pn532_init(i2c_inst_t *i2c);

int pn532_write_command(uint8_t cmd, const uint8_t *param, uint8_t len);
int pn532_read_response(uint8_t cmd, uint8_t *resp, uint8_t len);

const char *pn532_firmware_ver();

bool pn532_config_sam();
bool pn532_config_rf();

void pn532_rf_field(bool on);

bool pn532_poll_mifare(uint8_t uid[7], int *len);
bool pn532_poll_felica(uint8_t uid[8], uint8_t pmm[8], uint8_t syscode[2], bool from_cache);

bool pn532_mifare_auth(const uint8_t uid[4], uint8_t block_id, uint8_t key_id, const uint8_t key[6]);
bool pn532_mifare_read(uint8_t block_id, uint8_t block_data[16]);

bool pn532_felica_read(uint16_t svc_code, uint16_t block_id, uint8_t block_data[16]);
bool pn532_felica_write(uint16_t svc_code, uint16_t block_id, const uint8_t block_data[16]);

void pn532_select();
void pn532_deselect();

#endif
