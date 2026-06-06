// crc32.c
#include <stdint.h>
#include "crc32.h"

// Standard CRC32 polynomial
#define CRC32_POLY 0xEDB88320

uint32_t crc32(const uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC32_POLY;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}