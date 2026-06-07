/**
 * @file    vtfp_port.h
 * @brief   VTFP v1 port layer interface
 * @note    Each port (rt-thread, freertos, bare) provides these
 *          functions. The core runtime calls vtfp_port_now_ms() and
 *          vtfp_port_sleep_ms(). Initialization is handled by the
 *          RTOS boot sequence (not by vtfp).
 */

#ifndef __VTFP_PORT_H
#define __VTFP_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Get current monotonic time in milliseconds.
 * @return ms since some arbitrary epoch (typically boot).
 */
uint32_t vtfp_port_now_ms(void);

/**
 * @brief  Sleep for the given number of milliseconds.
 *         Implementations should yield the CPU to other tasks.
 */
void vtfp_port_sleep_ms(uint32_t ms);

/**
 * @brief  Optional: do any RTOS-specific initialization for the port
 *         (e.g., create a dedicated thread for vtfp_poll).
 *         Default implementation is a no-op.
 */
void vtfp_port_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_PORT_H */
