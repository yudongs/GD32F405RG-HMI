/**
 * @file    test_vtfp_checksum.c
 * @brief   Host-pc tests for vtfp_checksum module
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vtfp/vtfp_types.h"
#include "vtfp_checksum.h"
#include "vtfp_header.h"

static int failures = 0;

#define EXPECT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    else        { printf("OK:   %s\n", msg); }                      \
} while (0)

int main(void) {
    printf("=== vtfp_checksum test ===\n");

    /* Test 1: write then verify a known header */
    vtfp_header_t hdr = {0};
    hdr.magic    = VTFP_MAGIC;
    hdr.version  = VTFP_VERSION;
    hdr.features = 0x0007;
    hdr.command  = 0x42;
    hdr.result   = 0x07;
    hdr.seq      = 0x12345678;
    hdr.param    = 0xDEADBEEF;
    hdr.data_addr = 0x20010000;
    hdr.data_len  = 0x100;
    hdr.flags    = 0xAA55AA55;
    hdr.checksum = 0;
    hdr.reserved = 0;

    vtfp_checksum_write(&hdr);
    EXPECT(hdr.checksum != 0, "checksum_write produces non-zero checksum");
    EXPECT(vtfp_checksum_verify(&hdr) == 0, "verify returns 0 after write");

    /* Test 2: tampered field causes verify failure */
    hdr.command = 0x43;  /* tamper */
    EXPECT(vtfp_checksum_verify(&hdr) != 0, "verify returns -1 after tamper");

    /* Test 3: NULL pointer handling */
    EXPECT(vtfp_checksum_verify(NULL) == -1, "verify(NULL) returns -1");
    vtfp_checksum_write(NULL);  /* should not crash */
    EXPECT(1, "write(NULL) does not crash");

    /* Test 4: known value — header.bin from M1 cross-lang test
     * (commit 5c288ce) had checksum 0xEB79735A. We don't have that
     * file here, so we just verify consistency. */

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll vtfp_checksum checks passed.\n");
    return 0;
}
