/**
 * @file    vtfp_compat.h
 * @brief   AC5 (ARM Compiler 5) compatibility shim for the VTFP v1 runtime
 * @note    MUST be included BEFORE any <vtfp/...> header (used as --preinclude).
 *
 *   1. AC5 lacks C11 _Static_assert. Polyfill with the negative-array trick.
 *   2. AC5 ignores __attribute__((weak)) semantics — see vtfp_port_reorder.c.
 *   3. Pulls in vtfp_result.h so handlers can use either VTFP_R_* or VTFP_ERR_*.
 *
 * The weak-attribute polyfill was removed (it broke every other __attribute__
 * use such as packed/aligned/section). The vtfp_port_now_ms() duplicate-symbol
 * problem is resolved by vtfp_port_reorder.c: it provides a single strong
 * FreeRTOS-backed implementation and the weak fallback in vtfp_core.c is
 * defeated at link time by reordering the .o files (vtfp_port_freertos.o comes
 * FIRST in the linker line, before vtfp_core.o).
 */
#ifndef __VTFP_COMPAT_H
#define __VTFP_COMPAT_H

/* Polyfill _Static_assert for ARM Compiler 5 (no C11 support).
 * Uses a negative-array typedef trick that fails at compile time. */
#ifndef _Static_assert
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define _Static_assert(cond, msg) \
    typedef char vtfp_sa_##__LINE__[(cond) ? 1 : -1]
#else
#include <assert.h>  /* fallback: runtime assert */
#define _Static_assert(cond, msg) assert(cond)
#endif
#endif

/* Result code aliases (vtfp_result.h defines VTFP_R_*; vtfp_header.h defines VTFP_ERR_*).
 * If you prefer VTFP_R_* naming in handlers, include this header. */
#include "vtfp_result.h"

#endif /* __VTFP_COMPAT_H */
