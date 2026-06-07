/**
 * @file    vtfp_checksum.c
 * @brief   Header CRC-32 verification wrapper
 * @note    Bridges vtfp-protocol's vtfp_crc32 with the vtfp_header_t struct.
 *          Coverage: bytes [0..35] (everything before checksum + reserved).
 */

#include "vtfp_checksum.h"
#include "vtfp_crc.h"
#include "vtfp_header.h"  /* for VTFP_HEADER_SIZE and vtfp_crc32 */

#define CHECKSUM_COVERED_BYTES  36  /* magic..flags = offset 0..35 */

int32_t vtfp_checksum_verify(const vtfp_header_t *hdr) {
    if (hdr == NULL) return -1;
    uint32_t computed = vtfp_crc32(hdr, CHECKSUM_COVERED_BYTES);
    if (computed != hdr->checksum) return -1;
    return 0;
}

void vtfp_checksum_write(vtfp_header_t *hdr) {
    if (hdr == NULL) return;
    /* checksum field must be 0 before computing (otherwise we'd include
     * the previous checksum in the new computation) */
    hdr->checksum = 0;
    hdr->checksum = vtfp_crc32(hdr, CHECKSUM_COVERED_BYTES);
}
