/**
 * @file    test_safety.c
 * @brief   Verify safety key, ARM timeout, and flags bit assignments
 */

#include <stdio.h>
#include "vtfp_safety.h"

static int failures = 0;

#define EXPECT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    else        { printf("OK:   %s\n", msg); }                      \
} while (0)

int main(void) {
    printf("=== safety primitives test ===\n");

    EXPECT(VTFP_SAFETY_KEY == 0xAA55AA55UL, "default safety key is 0xAA55AA55");
    EXPECT(VTFP_ARM_TIMEOUT_MS == 2000U, "default ARM timeout is 2000ms");

    /* Flags bits are non-overlapping */
    EXPECT((VTFP_FLAGS_ARMED_BIT & VTFP_FLAGS_AUTO_DISARM_BIT) == 0, "ARMED and AUTO_DISARM bits don't overlap");

    /* Helpers */
    EXPECT(vtfp_flags_safety_key(0xDEADBEEFUL) == 0xDEADBEEFUL, "flags safety key extract");
    EXPECT(vtfp_flags_auto_disarm(VTFP_FLAGS_AUTO_DISARM_BIT) == 1, "auto_disarm set when bit set");
    EXPECT(vtfp_flags_auto_disarm(0) == 0, "auto_disarm clear when bit clear");
    EXPECT(vtfp_flags_auto_disarm(VTFP_FLAGS_ARMED_BIT) == 0, "auto_disarm not confused with ARMED bit");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll safety checks passed.\n");
    return 0;
}
