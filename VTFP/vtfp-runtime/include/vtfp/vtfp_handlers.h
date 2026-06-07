/**
 * @file    vtfp_handlers.h
 * @brief   VTFP v1 handler registration API
 * @note    User implements handlers matching vtfp_handler_fn signature,
 *          then calls vtfp_register_handler() in their init code.
 */

#ifndef __VTFP_HANDLERS_H
#define __VTFP_HANDLERS_H

#include "vtfp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Register a handler for a command ID.
 * @param  cmd       command ID (0x01..0x0F for user commands)
 * @param  fn        handler function (NULL to unregister)
 * @param  policy    ARM requirement
 * @return 0 on success, -1 if cmd is out of range or table is full
 */
int32_t vtfp_register_handler(uint8_t cmd, vtfp_handler_fn fn, vtfp_arm_policy_t policy);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_HANDLERS_H */
