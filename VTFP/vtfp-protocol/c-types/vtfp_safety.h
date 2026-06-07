/**
 * @file    vtfp_safety.h
 * @brief   VTFP v1 safety primitives: safety key, ARM timeout, flags bits
 * @note    Pure constants — the ARM *state machine* (tick-based arm/disarm
 *          enforcement) lives in M2 firmware (vtfp_core.c). This header
 *          only defines the values PC and MCU must agree on.
 */

#ifndef __VTFP_SAFETY_H
#define __VTFP_SAFETY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default safety key (PC sends in param when issuing VTFP_CMD_ARM) */
#define VTFP_SAFETY_KEY            0xAA55AA55UL

/* Default ARM timeout in milliseconds (overridable in vtfp.yaml) */
#define VTFP_ARM_TIMEOUT_MS        2000U

/* header.flags bit assignments */
#define VTFP_FLAGS_SAFETY_KEY_MASK 0xFFFFFFFFUL  /* when written by MCU, holds last valid key */
#define VTFP_FLAGS_ARMED_BIT       (1U << 0)     /* MCU sets this when arm_tick is active */
#define VTFP_FLAGS_AUTO_DISARM_BIT (1U << 1)     /* MCU auto-disarms after each dangerous cmd */
#define VTFP_FLAGS_RESERVED        0xFFFFFFFCUL  /* bits 2..31 reserved for future use */

/* Helper: extract safety key from header.flags (MCU-side) */
static inline uint32_t vtfp_flags_safety_key(uint32_t flags) {
    return flags & VTFP_FLAGS_SAFETY_KEY_MASK;
}

/* Helper: check if auto-disarm is enabled */
static inline int vtfp_flags_auto_disarm(uint32_t flags) {
    return (flags & VTFP_FLAGS_AUTO_DISARM_BIT) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_SAFETY_H */
