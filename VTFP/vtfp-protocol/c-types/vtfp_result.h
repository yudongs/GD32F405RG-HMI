/**
 * @file    vtfp_result.h
 * @brief   VTFP v1 result codes written to header.result by MCU
 * @note    Result codes are 8-bit values. 0x00 = OK. Non-zero = error.
 *          PC SDK translates these into Python exceptions.
 */

#ifndef __VTFP_RESULT_H
#define __VTFP_RESULT_H

#include <stdint.h>
#include "vtfp_header.h"  /* for VTFP_OK / VTFP_ERR_* aliases */

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical result code values (re-declared for completeness) */
#define VTFP_R_OK                  0U
#define VTFP_R_NOT_ARMED           1U
#define VTFP_R_ARM_EXPIRED         2U
#define VTFP_R_SAFETY_KEY          3U
#define VTFP_R_PARAM_RANGE         4U
#define VTFP_R_NOT_INIT            5U
#define VTFP_R_BUF_OVERFLOW        6U
#define VTFP_R_UNKNOWN_CMD         0xFFU

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_RESULT_H */
