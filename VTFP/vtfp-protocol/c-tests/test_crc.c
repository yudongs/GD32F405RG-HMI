/**
 * @file    test_crc.c
 * @brief   CRC-32 known-answer tests
 * @note    Polynomial 0xEDB88320 (reflected). Check value for "123456789"
 *          is 0xCBF43926. Implementation lives in c-types/vtfp_crc.c.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
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

int main(void) {
    printf("=== CRC-32 test ===\n");

    /* Known answer #1: standard check value */
    const char *check_str = "123456789";
    EXPECT_EQ(vtfp_crc32(check_str, 9), 0xCBF43926U, "CRC of '123456789'");

    /* Known answer #2: empty input */
    EXPECT_EQ(vtfp_crc32("", 0), 0x00000000U, "CRC of empty buffer");

    /* Known answer #3: single byte 'a' */
    EXPECT_EQ(vtfp_crc32("a", 1), 0xE8B7BE43U, "CRC of 'a'");

    /* Determinism: same input twice gives same output */
    const char *msg = "VTFP v1";
    uint32_t c1 = vtfp_crc32(msg, strlen(msg));
    uint32_t c2 = vtfp_crc32(msg, strlen(msg));
    EXPECT_EQ(c1, c2, "CRC is deterministic");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll CRC checks passed.\n");
    return 0;
}
