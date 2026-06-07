/**
 * @file    test_cross_lang.c
 * @brief   Read a 44-byte header emitted by emit_header.py and verify
 *          field-by-field. Proves Python pack and C struct agree on
 *          wire format.
 *
 * Run sequence (from Makefile):
 *   1. python emit_header.py header.bin
 *   2. compile this C program
 *   3. run ./test_cross_lang header.bin
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vtfp_header.h"
#include "vtfp_crc.h"

static int failures = 0;

#define EXPECT_EQ(actual, expected, msg) do {                              \
    uint32_t _a = (actual), _e = (expected);                              \
    if (_a != _e) {                                                        \
        fprintf(stderr, "FAIL: %s — got 0x%08X, expected 0x%08X\n",        \
                msg, _a, _e);                                              \
        failures++;                                                        \
    } else {                                                               \
        printf("OK:   %s = 0x%08X\n", msg, _a);                            \
    }                                                                      \
} while (0)

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <header.bin>\n", argv[0]);
        return 1;
    }

    /* The Makefile invokes emit_header.py as a separate step; if you
       run this binary directly, ensure header.bin already exists. */

    /* Step 1: read the blob */
    const char *blob_path = argv[1];
    FILE *fp = fopen(blob_path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", blob_path);
        return 1;
    }
    uint8_t buf[44];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (n != sizeof(buf)) {
        fprintf(stderr, "Expected 44 bytes, got %zu\n", n);
        return 1;
    }

    /* Step 2: copy into packed struct (compiler lays it out identically) */
    vtfp_header_t hdr;
    memcpy(&hdr, buf, sizeof(hdr));

    /* Step 3: field-by-field checks */
    printf("=== cross-language wire format test ===\n");
    EXPECT_EQ(hdr.magic,         VTFP_MAGIC,           "magic");
    EXPECT_EQ(hdr.version,       VTFP_VERSION,         "version");
    EXPECT_EQ(hdr.features,      0x0003,               "features");
    EXPECT_EQ(hdr.command,       0x00000042,           "command");
    EXPECT_EQ(hdr.result,        0x00000007,           "result");
    EXPECT_EQ(hdr.seq,           0x12345678,           "seq");
    EXPECT_EQ(hdr.param,         0xDEADBEEF,           "param");
    EXPECT_EQ(hdr.data_addr,     0x20010000,           "data_addr");
    EXPECT_EQ(hdr.data_len,      0x00000100,           "data_len");
    EXPECT_EQ(hdr.flags,         0xAA55AA55,           "flags");

    /* Step 4: verify checksum matches recomputed CRC over bytes [0..35] */
    uint32_t crc_computed = vtfp_crc32(buf, 36);
    EXPECT_EQ(crc_computed, hdr.checksum,             "checksum (recomputed)");

    /* Step 5: reserved must be 0 */
    EXPECT_EQ(hdr.reserved,      0,                    "reserved");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll cross-language checks passed.\n");
    return 0;
}
