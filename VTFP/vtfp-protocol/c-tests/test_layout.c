/**
 * @file    test_layout.c
 * @brief   Host-pc test: verify vtfp_header_t field offsets and size
 * @note    Compiled with `gcc -m32` (or 64-bit gcc; the offsets are absolute
 *          byte positions, not register-dependent). On failure, prints which
 *          field is at the wrong offset. Returns non-zero exit code.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "vtfp_header.h"

static int failures = 0;

#define EXPECT_OFFSET(field, expected) do {                                  \
    size_t _off = offsetof(vtfp_header_t, field);                            \
    if (_off != (expected)) {                                                \
        fprintf(stderr, "FAIL: %s offset = %zu, expected %zu\n",             \
                #field, _off, (size_t)(expected));                           \
        failures++;                                                          \
    } else {                                                                 \
        printf("OK:   %s @ offset %zu (%zu bytes)\n",                        \
               #field, _off, sizeof(((vtfp_header_t*)0)->field));            \
    }                                                                        \
} while (0)

#define EXPECT_SIZE(expected) do {                                          \
    size_t _sz = sizeof(vtfp_header_t);                                      \
    if (_sz != (expected)) {                                                 \
        fprintf(stderr, "FAIL: sizeof(vtfp_header_t) = %zu, expected %zu\n", \
                _sz, (size_t)(expected));                                    \
        failures++;                                                          \
    } else {                                                                 \
        printf("OK:   sizeof(vtfp_header_t) = %zu\n", _sz);                  \
    }                                                                        \
} while (0)

#define EXPECT_MAGIC(expected) do {                                          \
    if (VTFP_MAGIC != (expected)) {                                          \
        fprintf(stderr, "FAIL: VTFP_MAGIC = 0x%08lX, expected 0x%08lX\n",    \
                (unsigned long)VTFP_MAGIC, (unsigned long)(expected));       \
        failures++;                                                          \
    } else {                                                                 \
        printf("OK:   VTFP_MAGIC = 0x%08lX\n", (unsigned long)VTFP_MAGIC);   \
    }                                                                        \
} while (0)

int main(void) {
    printf("=== vtfp_header_t layout test ===\n");

    EXPECT_OFFSET(magic,       0);
    EXPECT_OFFSET(version,     4);
    EXPECT_OFFSET(features,    6);
    EXPECT_OFFSET(command,     8);
    EXPECT_OFFSET(result,     12);
    EXPECT_OFFSET(seq,        16);
    EXPECT_OFFSET(param,      20);
    EXPECT_OFFSET(data_addr,  24);
    EXPECT_OFFSET(data_len,   28);
    EXPECT_OFFSET(flags,      32);
    EXPECT_OFFSET(checksum,   36);
    EXPECT_OFFSET(reserved,   40);

    EXPECT_SIZE(44);
    EXPECT_MAGIC(0x50465456UL);

    if (failures) {
        fprintf(stderr, "\n%d layout check(s) failed\n", failures);
        return 1;
    }
    printf("\nAll layout checks passed.\n");
    return 0;
}
