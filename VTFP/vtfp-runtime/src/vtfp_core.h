/**
 * @file    vtfp_core.h
 * @brief   VTFP v1 runtime core (internal)
 * @note    NOT a public header. Users include <vtfp/vtfp.h> only.
 *          Exposes the dispatch table to vtfp_dispatch.c and the
 *          dispatch loop state to vtfp_poll().
 */

#ifndef __VTFP_CORE_INTERNAL_H
#define __VTFP_CORE_INTERNAL_H

#include "vtfp/vtfp.h"
#include "vtfp/vtfp_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dispatch table entry (one per command) */
typedef struct {
    vtfp_handler_fn   fn;        /* NULL = no handler */
    vtfp_arm_policy_t policy;    /* ARM_NONE or ARM_REQUIRED */
} vtfp_dispatch_entry_t;

/* The dispatch table (defined in vtfp_core.c, populated by vtfp_register_handler) */
extern vtfp_dispatch_entry_t g_dispatch[VTFP_MAX_HANDLERS];

/* The current configuration (defined in vtfp_core.c) */
extern vtfp_config_t g_config;

/* Pointer to the shared RAM header (computed in vtfp_init) */
extern volatile vtfp_header_t *g_header;

/* Pointer to the data segment (computed in vtfp_init) */
extern volatile uint8_t *g_data;

/* Get current monotonic time in ms (provided by port layer) */
extern uint32_t vtfp_port_now_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_CORE_INTERNAL_H */
