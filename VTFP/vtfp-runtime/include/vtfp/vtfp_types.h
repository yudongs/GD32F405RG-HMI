/**
 * @file    vtfp_types.h
 * @brief   VTFP v1 runtime types — config, request, response, handler signature
 * @note    Pure declarations, no implementation. RTOS-agnostic.
 *          Mirrors the wire-format contract from vtfp-protocol/vtfp_header.h.
 */

#ifndef __VTFP_TYPES_H
#define __VTFP_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "vtfp_header.h"  /* for vtfp_header_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of commands in the dispatch table (0x01..0x0F = 15 user slots).
 * Plus 4 standard commands + 2 safety = up to 21 total handlers. */
#define VTFP_MAX_HANDLERS         32U

/* Command ID partitions — mirrors vtfp_command.h */
#define VTFP_CMD_USER_BASE        0x01U
#define VTFP_CMD_USER_MAX         0x0FU
#define VTFP_CMD_STD_BASE         0x10U
#define VTFP_CMD_STD_MAX          0x1FU

/**
 * @brief  Runtime configuration. Set by user, passed to vtfp_init().
 */
typedef struct {
    /* Memory layout */
    uint32_t header_base;        /* SRAM address of vtfp_header_t (default 0x20000000) */
    uint32_t data_addr;          /* SRAM address of .vtfp_data segment */
    uint32_t data_size;          /* size of .vtfp_data in bytes (typically 1024) */

    /* Behavior */
    uint32_t poll_period_ms;     /* main loop period in ms (RTOS-specific, just a hint) */
    uint32_t arm_timeout_ms;     /* default 2000ms */
    uint32_t safety_key;         /* default 0xAA55AA55 */
    bool     auto_disarm;        /* default true */
} vtfp_config_t;

/**
 * @brief  Request from PC (read by handler).
 */
typedef struct {
    uint32_t seq;                /* transaction ID from header.seq */
    uint32_t param;              /* command-specific argument from header.param */
    const uint8_t *data;         /* pointer to .vtfp_data, length = data_len */
    uint32_t data_len;           /* bytes in .vtfp_data */
} vtfp_request_t;

/**
 * @brief  Response to PC (written by handler).
 */
typedef struct {
    uint32_t result;             /* result code (VTFP_OK = 0, non-zero = error) */
    uint8_t *data;               /* pointer to .vtfp_data, length = data_len */
    uint32_t data_len;           /* bytes written to .vtfp_data */
} vtfp_response_t;

/**
 * @brief  Handler function signature.
 * @param  req     request from PC
 * @param  resp    response to write (handler sets result and data)
 * @return 0 on success, non-zero to override resp.result
 */
typedef int32_t (*vtfp_handler_fn)(const vtfp_request_t *req, vtfp_response_t *resp);

/**
 * @brief  ARM policy for a command.
 */
typedef enum {
    VTFP_ARM_NONE   = 0,        /* safe command, no ARM needed */
    VTFP_ARM_REQUIRED = 1,      /* dangerous command, requires ARM */
} vtfp_arm_policy_t;

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_TYPES_H */
