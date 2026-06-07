/**
 * @file    test_vtfp_arm.c
 * @brief   Host-pc tests for vtfp_arm state machine
 */

#include <stdio.h>
#include "vtfp_arm.h"
#include "vtfp/vtfp_types.h"

static int failures = 0;

#define EXPECT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    else        { printf("OK:   %s\n", msg); }                      \
} while (0)

int main(void) {
    printf("=== vtfp_arm test ===\n");

    /* Initialize with default 2000ms timeout and 0xAA55AA55 key */
    vtfp_arm_init(2000, 0xAA55AA55, true);
    EXPECT(vtfp_arm_get_state() == VTFP_ARM_STATE_DISARMED, "initial state is DISARMED");
    EXPECT(vtfp_arm_is_armed(0) == 0, "not armed at t=0");

    /* Try ARM with wrong key */
    EXPECT(vtfp_arm_try_arm(0xDEADBEEF, 100) == -1, "wrong key returns -1");
    EXPECT(vtfp_arm_get_state() == VTFP_ARM_STATE_DISARMED, "still DISARMED after wrong key");

    /* Try ARM with correct key */
    EXPECT(vtfp_arm_try_arm(0xAA55AA55, 100) == 0, "correct key returns 0");
    EXPECT(vtfp_arm_get_state() == VTFP_ARM_STATE_ARMED, "ARMED after correct key");
    EXPECT(vtfp_arm_is_armed(100) == 1, "is_armed returns 1 within window");

    /* Expiration: now_ms = 100 + 2000 = 2100 is past the window */
    EXPECT(vtfp_arm_is_armed(2100) == 0, "is_armed returns 0 after expiration");
    EXPECT(vtfp_arm_get_state() == VTFP_ARM_STATE_DISARMED, "auto-expires to DISARMED");

    /* Disarm */
    vtfp_arm_try_arm(0xAA55AA55, 5000);
    EXPECT(vtfp_arm_get_state() == VTFP_ARM_STATE_ARMED, "ARMED after second try");
    vtfp_arm_disarm();
    EXPECT(vtfp_arm_get_state() == VTFP_ARM_STATE_DISARMED, "DISARMED after explicit disarm");

    /* Init re-applies defaults (overriding custom values) */
    vtfp_arm_init(5000, 0x12345678, false);
    EXPECT(vtfp_arm_try_arm(0xAA55AA55, 0) == -1, "after re-init, old key fails");
    EXPECT(vtfp_arm_try_arm(0x12345678, 0) == 0, "after re-init, new key works");
    EXPECT(vtfp_arm_is_armed(4500) == 1, "new timeout (5000ms) — armed at t=4500");
    EXPECT(vtfp_arm_is_armed(5000) == 0, "new timeout (5000ms) — expired at t=5000");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll vtfp_arm checks passed.\n");
    return 0;
}
