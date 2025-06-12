#ifndef SPAD0_H
#define SPAD0_H

#include <stddef.h>
#include <stdint.h>

void spad0_decrypt(const uint8_t* input, uint8_t* output, size_t len);

#endif