/**
 * @file    vtfp_dispatch.h
 * @brief   VTFP v1 standard command handlers
 * @note    Handles VTFP_CMD_QUERY_INFO (0x10) and VTFP_CMD_QUERY_STATE (0x11).
 *          User-defined commands (0x01..0x0F) are dispatched via g_dispatch
 *          in vtfp_core.c, not here. M2-T7 codegen produces a similar
 *          file for project-specific extensions.
 */

#ifndef __VTFP_DISPATCH_H
#define __VTFP_DISPATCH_H

#include "vtfp/vtfp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Handle a VTFP standard command (0x10..0x1F).
 * @param  cmd_id  the command ID (e.g., VTFP_CMD_QUERY_INFO = 0x10)
 * @param  req     request from PC
 * @param  resp    response to write
 * @return 0 if handled, -1 if not a recognized standard command
 */
int32_t vtfp_dispatch_standard(uint8_t cmd_id, const vtfp_request_t *req, vtfp_response_t *resp);

/**
 * @brief  Get the runtime features bitmap (for QUERY_INFO).
 */
uint16_t vtfp_runtime_features(void);

/**
 * @brief  Get the product ID string (for QUERY_INFO).
 */
const char *vtfp_runtime_product_id(void);

/**
 * @brief  Get the firmware version string (for QUERY_INFO).
 */
const char *vtfp_runtime_fw_version(void);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_DISPATCH_H */
