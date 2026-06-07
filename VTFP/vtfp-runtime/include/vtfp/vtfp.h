/**
 * @file    vtfp.h
 * @brief   VTFP v1 runtime top-level API
 * @note    This is the only header user code needs to include.
 *          For handler implementation, also include vtfp_handlers.h.
 *
 * Usage (RT-Thread main loop):
 * @code
 *   #include <vtfp/vtfp.h>
 *
 *   static int32_t my_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
 *       // ...
 *       return 0;
 *   }
 *
 *   int main(void) {
 *       vtfp_config_t cfg = {
 *           .header_base = 0x20000000,
 *           .data_addr   = 0x20000100,  // after the header
 *           .data_size   = 1024,
 *           .poll_period_ms = 20,
 *           .arm_timeout_ms = 2000,
 *           .safety_key     = 0xAA55AA55,
 *           .auto_disarm     = true,
 *       };
 *       vtfp_init(&cfg);
 *       vtfp_register_handler(0x01, my_handler, VTFP_ARM_NONE);
 *
 *       while (1) {
 *           vtfp_poll();
 *           rt_thread_mdelay(cfg.poll_period_ms);
 *       }
 *   }
 * @endcode
 */

#ifndef __VTFP_H
#define __VTFP_H

#include "vtfp_types.h"
#include "vtfp_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime version (mirrors vtfp-protocol/VERSION) */
#define VTFP_RUNTIME_VERSION_MAJOR  0
#define VTFP_RUNTIME_VERSION_MINOR  1
#define VTFP_RUNTIME_VERSION_PATCH  0

/**
 * @brief  Initialize the runtime. Call once at startup.
 * @param  cfg  configuration (NULL for defaults)
 * @return 0 on success, -1 on configuration error
 */
int32_t vtfp_init(const vtfp_config_t *cfg);

/**
 * @brief  Poll the shared RAM for incoming commands. Call from main loop.
 * @return 0 if no command was processed, 1 if a command was processed
 */
int32_t vtfp_poll(void);

/**
 * @brief  Get the runtime version string (e.g. "0.1.0").
 */
const char *vtfp_version(void);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_H */
