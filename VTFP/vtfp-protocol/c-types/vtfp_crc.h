/**
 * @file    vtfp_crc.h
 * @brief   VTFP v1 CRC-32 (IEEE 802.3 polynomial 0xEDB88320, reflected)
 * @note    Must match Python `vtfp_proto.crc.crc32()` byte-for-byte.
 *            poly   = 0xEDB88320  (reflected form of 0x04C11DB7)
 *            init   = 0xFFFFFFFF
 *            refin  = True
 *            refout = True
 *            xorout = 0xFFFFFFFF
 *            check  = 0xCBF43926  (CRC of "123456789")
 */

#ifndef __VTFP_CRC_H
#define __VTFP_CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Compute CRC-32 over a byte buffer
 * @param  data  Input buffer
 * @param  len   Number of bytes
 * @return 32-bit CRC (reflected, XORed with 0xFFFFFFFF)
 */
uint32_t vtfp_crc32(const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_CRC_H */
