/**
 * @file    vtfp_arm.c
 * @brief   ARM safety state machine implementation
 * @note    Tracks a single global "armed" flag with timeout.
 *          The auto_disarm flag controls whether the flag is cleared
 *          after a dangerous command succeeds.
 */

#include "vtfp_arm.h"

static struct {
    vtfp_arm_state_t state;     /* current ARM state */
    uint32_t        arm_tick;   /* when the ARM was set (now_ms at arm time) */
    uint32_t        timeout_ms; /* how long the ARM remains valid */
    uint32_t        safety_key; /* expected key */
    bool            auto_disarm;/* clear after each dangerous cmd */
} s_arm = {
    .state = VTFP_ARM_STATE_DISARMED,
    .arm_tick = 0,
    .timeout_ms = 2000,
    .safety_key = 0xAA55AA55,
    .auto_disarm = true,
};

void vtfp_arm_init(uint32_t timeout_ms, uint32_t safety_key, bool auto_disarm) {
    s_arm.state = VTFP_ARM_STATE_DISARMED;
    s_arm.arm_tick = 0;
    s_arm.timeout_ms = timeout_ms ? timeout_ms : 2000;
    s_arm.safety_key = safety_key ? safety_key : 0xAA55AA55;
    s_arm.auto_disarm = auto_disarm;
}

int32_t vtfp_arm_try_arm(uint32_t key, uint32_t now_ms) {
    if (key != s_arm.safety_key) {
        return -1;  /* wrong key */
    }
    s_arm.state = VTFP_ARM_STATE_ARMED;
    s_arm.arm_tick = now_ms;
    return 0;
}

void vtfp_arm_disarm(void) {
    s_arm.state = VTFP_ARM_STATE_DISARMED;
    s_arm.arm_tick = 0;
}

int32_t vtfp_arm_is_armed(uint32_t now_ms) {
    if (s_arm.state != VTFP_ARM_STATE_ARMED) {
        return 0;
    }
    /* Check expiration (using uint32_t subtraction for wraparound safety) */
    uint32_t elapsed = now_ms - s_arm.arm_tick;
    if (elapsed >= s_arm.timeout_ms) {
        s_arm.state = VTFP_ARM_STATE_DISARMED;  /* auto-expire */
        s_arm.arm_tick = 0;
        return 0;
    }
    return 1;
}

vtfp_arm_state_t vtfp_arm_get_state(void) {
    return s_arm.state;
}
