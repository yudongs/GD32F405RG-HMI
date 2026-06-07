/**
 * @file    vtfp_crc.c
 * @brief   VTFP v1 CRC-32 (IEEE 802.3 polynomial 0xEDB88320, reflected)
 * @note    Must match Python `vtfp_proto.crc.crc32()` byte-for-byte.
 *            poly   = 0xEDB88320  (reflected form of 0x04C11DB7)
 *            init   = 0xFFFFFFFF
 *            refin  = True
 *            refout = True
 *            xorout = 0xFFFFFFFF
 *            check  = 0xCBF43926  (CRC of "123456789")
 */

#include "vtfp_crc.h"

static uint32_t crc32_update(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        uint32_t mask = -(crc & 1);
        crc = (crc >> 1) ^ (0xEDB88320U & mask);
    }
    return crc;
}

uint32_t vtfp_crc32(const void *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFUL;
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_update(crc, p[i]);
    }
    return crc ^ 0xFFFFFFFFUL;
}
