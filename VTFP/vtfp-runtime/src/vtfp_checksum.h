/**
 * @file    vtfp_checksum.h
 * @brief   VTFP v1 header CRC-32 verification
 * @note    Wraps vtfp_crc32 (from vtfp-protocol). Computes CRC over
 *          header bytes [0..35] (excluding checksum and reserved fields)
 *          and compares to header.checksum.
 */

#ifndef __VTFP_CHECKSUM_H
#define __VTFP_CHECKSUM_H

#include "vtfp/vtfp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Verify header.checksum matches CRC-32 of bytes [0..35].
 * @param  hdr  pointer to vtfp_header_t (read-only)
 * @return 0 if valid, -1 if mismatch
 */
int32_t vtfp_checksum_verify(const vtfp_header_t *hdr);

/**
 * @brief  Compute and store CRC-32 over header bytes [0..35] into hdr->checksum.
 * @param  hdr  pointer to vtfp_header_t (will be modified)
 */
void vtfp_checksum_write(vtfp_header_t *hdr);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_CHECKSUM_H */
