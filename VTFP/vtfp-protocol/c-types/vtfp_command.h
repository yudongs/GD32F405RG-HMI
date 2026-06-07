/**
 * @file    vtfp_command.h
 * @brief   VTFP v1 command ID partition and standard command set
 * @note    Command IDs are 8-bit values in header.command. The partition
 *          map is enforced by convention; vtfp_dispatch.c (M2) is the only
 *          place that interprets unknown IDs.
 */

#ifndef __VTFP_COMMAND_H
#define __VTFP_COMMAND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Partition map ---- */
#define VTFP_CMD_USER_BASE          0x01U  /* 0x01..0x0F: domain-specific (codegen) */
#define VTFP_CMD_USER_MAX           0x0FU
#define VTFP_CMD_STD_BASE           0x10U  /* 0x10..0x1F: VTFP standard */
#define VTFP_CMD_STD_MAX            0x1FU
#define VTFP_CMD_SAFETY_BASE        0xF0U  /* 0xF0..0xFF: safety / control */
#define VTFP_CMD_SAFETY_MAX         0xFFU

/* ---- VTFP standard commands (always available) ---- */
#define VTFP_CMD_QUERY_INFO         0x10U  /* PC→MCU; resp: {product_id, fw_version, protocol_version, features} */
#define VTFP_CMD_QUERY_STATE        0x11U  /* PC→MCU; resp: 8-byte state word */
#define VTFP_CMD_RESET              0x12U  /* PC→MCU; requires ARM; warm reset */
#define VTFP_CMD_HEARTBEAT          0x13U  /* PC→MCU; liveness ping (no ARM) */

/* ---- Safety / control commands ---- */
#define VTFP_CMD_ARM                0xFEU  /* PC→MCU; param = safety_key */
#define VTFP_CMD_DISARM             0xFFU  /* PC→MCU; clears arm_tick */

/* ---- Helpers ---- */
static inline int vtfp_cmd_is_user(uint8_t cmd)    { unsigned int c = cmd; return c >= VTFP_CMD_USER_BASE  && c <= VTFP_CMD_USER_MAX;  }
static inline int vtfp_cmd_is_standard(uint8_t cmd) { unsigned int c = cmd; return c >= VTFP_CMD_STD_BASE   && c <= VTFP_CMD_STD_MAX;   }
static inline int vtfp_cmd_is_safety(uint8_t cmd)  { unsigned int c = cmd; return c >= VTFP_CMD_SAFETY_BASE && c <= VTFP_CMD_SAFETY_MAX; }

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_COMMAND_H */
