/**
 * @file    vtfp_arm.h
 * @brief   VTFP v1 ARM safety state machine
 * @note    Tracks whether a dangerous command may execute. Decoupled
 *          from any RTOS — caller provides "now_ms" ticks.
 */

#ifndef __VTFP_ARM_H
#define __VTFP_ARM_H

#include "vtfp/vtfp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ARM state enum (for debugging/logging) */
typedef enum {
    VTFP_ARM_STATE_DISARMED = 0,
    VTFP_ARM_STATE_ARMED    = 1,
} vtfp_arm_state_t;

/**
 * @brief  Initialize the ARM state machine.
 * @param  timeout_ms     how long an ARM remains valid (default 2000)
 * @param  safety_key     the 32-bit safety key
 * @param  auto_disarm    if true, clear ARM after each dangerous command
 */
void vtfp_arm_init(uint32_t timeout_ms, uint32_t safety_key, bool auto_disarm);

/**
 * @brief  Attempt to ARM with the given key. Sets internal state to ARMED if key matches.
 * @param  key        the safety key sent by PC
 * @param  now_ms     current monotonic time in ms (caller-provided)
 * @return 0 on success, -1 if key mismatch
 */
int32_t vtfp_arm_try_arm(uint32_t key, uint32_t now_ms);

/**
 * @brief  Explicitly disarm (e.g., on DISARM command).
 */
void vtfp_arm_disarm(void);

/**
 * @brief  Check if a dangerous command is allowed right now.
 * @param  now_ms     current monotonic time in ms
 * @return 1 if armed and not expired, 0 otherwise
 */
int32_t vtfp_arm_is_armed(uint32_t now_ms);

/**
 * @brief  Get the current state (for logging).
 */
vtfp_arm_state_t vtfp_arm_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_ARM_H */
